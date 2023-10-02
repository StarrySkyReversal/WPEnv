#include "framework.h"
#include "ServiceSource.h"
#include "DownloadThread.h"
#include "Log.h"
#include "ProgressBarControls.h"
#include "StaticLabelControls.h"
#include "RichEditControls.h"
#include <algorithm>
#include "BaseFileOpt.h"
#include "Compression.h"
#include "Common.h"
#include "DownloadQueue.h"
#include "CircularQueue.h"
#include "FileModify.h"
#include "FileFindOpt.h"
#include "ServiceUse.h"
#include "CurlRequestOpt.h"
#include <openssl/crypto.h>
#include "StackOpt.h"

#define MAX_THREAD_NUM 8

unsigned long long totalSize;
unsigned long long downlodedTotalSize = 0;

int numDownloadThreadsSize = 0;
int numDynamicSubPartSize = 0;
int abnormalCount = 0;
int abnormalCloseThreadCount = 0;
int numLockFlow = 0;
int numLockFlowMax = 16;

bool bStartMonitor = false;

HANDLE hEvent;

Queue* workerQueueArray;

Stack* threadStackArray;

DownloadPart** globalPartGroup;

ThreadManager** threadManager;

void freeDownloadPart(DownloadPart* part) {
    if (part) {
        free((void*)part->version);
        free((void*)part->url);
        free((void*)part->filename);
        free((void*)part->filepath);
    }
}

DownloadPart* GenerateDownloadPartModel(
    const wchar_t* version,
    const wchar_t* link,
    ULONGLONG start,
    ULONGLONG end,
    const wchar_t* fileName = L"\0",
    const wchar_t* filePath = L"\0",
    ULONGLONG readBytes = 0
) {

    DownloadPart* part = (DownloadPart*)malloc(sizeof(DownloadPart));

    if (fileName[0] == L'\0') {
        wchar_t TempFilename[256] = { '\0' };
        swprintf_s(TempFilename, sizeof(TempFilename) / sizeof(wchar_t), L"%ls_part_%llu_%llu", version, start, end);
        part->filename = _wcsdup(TempFilename);
    }
    else {
        part->filename = _wcsdup(fileName);
    }

    if (filePath[0] == L'\0') {
        wchar_t TempFilePath[256] = { '\0' };
        swprintf_s(TempFilePath, sizeof(TempFilePath) / sizeof(wchar_t), L"%ls/%ls", DIRECTORY_DOWNLOAD, part->filename);
        part->filepath = _wcsdup(TempFilePath);
    }
    else {
        part->filepath = _wcsdup(filePath);
    }

    part->version = _wcsdup(version);
    part->url = _wcsdup(link);
    //part->indexCategory = GetCircularVal(&circularQueueArray);
    //part->indexCategory = 0;
    //part->retryCount = 0;
    //part->abnormalCount = 0;
    part->startByte = start;
    part->endByte = end;
    part->readBytes = readBytes;
    part->currentStartByte = part->startByte + part->readBytes;
    part->totalBytesLength = part->endByte - part->startByte + 1;
    part->status = 0;
    part->statusCode = -1;
    part->timestamp = 0;

    return part;
}

bool isDone() {
    EnterCriticalSection(&progressCriticalSection);
    bool done = downlodedTotalSize >= totalSize;
    LeaveCriticalSection(&progressCriticalSection);
    return done;
}

DWORD WINAPI DaemonMonitorThread(LPVOID param) {
    while (true) {
        if (isDone()) {
            Log("DaemonMonitorThread exit%d\r\n");
            break;
        }

        WaitForSingleObject(hEvent, INFINITE);

        if (bStartMonitor == true) {    // Start monitor thread state
            EnterCriticalSection(&progressCriticalSection);
            if (abnormalCount > 1) {
                numLockFlowMax -= abnormalCount;
                abnormalCount = 0;
            }
            else {
                numLockFlowMax += 1;
            }

            if (numLockFlowMax < 16) {
                numLockFlowMax = 16;
            }

            if (numLockFlowMax > 128) {
                numLockFlowMax = 128;
            }

            Log("numLockFlowMax:%d\r\n", numLockFlowMax);

            LeaveCriticalSection(&progressCriticalSection);
        }

        ResetEvent(hEvent);
    }

    return 0;
}

DWORD WINAPI DaemonDownloadThread(LPVOID param) {
    int threadIndex = *((int*)param);

    while (true) {
        EnterCriticalSection(&progressCriticalSection);
        if (threadManager[threadIndex]->status == -1) {  // advance break
            Log("Abnormal advance break; index:%d;\r\n", threadIndex);

            threadManager[threadIndex]->status = 0;
            LeaveCriticalSection(&progressCriticalSection);
            break;
        }
        LeaveCriticalSection(&progressCriticalSection);

        if (isDone()) {
            Log("DaemonDownloadThread exit index: %d\r\n", threadIndex);
            break;
        }

        int counterSizeOrIndex = 0;
        DownloadPart** partGroup = (DownloadPart**)malloc(sizeof(DownloadPart*));

        EnterCriticalSection(&progressCriticalSection);
        while (counterSizeOrIndex < numDynamicSubPartSize) {
            if (numLockFlow >= numLockFlowMax) {
                break;
            }

            DownloadPart tempPart;
            if (dequeue(workerQueueArray, &tempPart)) {
                numLockFlow += 1;

                int tempSize = (counterSizeOrIndex + 1);
                partGroup = (DownloadPart**)realloc(partGroup, tempSize * sizeof(DownloadPart*));

                globalPartGroup[tempPart.index]->statusCode = -1;
                globalPartGroup[tempPart.index]->timestamp = GetTickCount64();
                partGroup[counterSizeOrIndex] = globalPartGroup[tempPart.index];

                counterSizeOrIndex += 1;
            }
            else {
                break;
            }
        }

        LeaveCriticalSection(&progressCriticalSection);

        if (counterSizeOrIndex > 0) {
            CurlMultipleDownloadThread(partGroup, counterSizeOrIndex);

            for (int i = 0; i < counterSizeOrIndex; i++) {
                if (partGroup[i]->status == -1) {
                    EnterCriticalSection(&progressCriticalSection);
                    partGroup[i]->status = 0;   // reset retry mark
                    abnormalCount += 1;
                    enqueue(workerQueueArray, *partGroup[i]);
                    LeaveCriticalSection(&progressCriticalSection);
                }
            }

            SetEvent(hEvent);
        }

        free(partGroup);
    }

    free(param);

    return 0;
}

DWORD WINAPI ProgressThread(LPVOID param) {
    SoftwareInfo* pSoftwareInfo = (SoftwareInfo*)param;

    wchar_t progressIngMsg[512];
    wchar_t progressFinishMsg[512];

    // init
    SetProgressBarPosition(0);
    UpdateStaticLabelInfo(L"");

    int currentProgress = 0;
    unsigned long long prevDownloadedTotalSize = 0;
    double nSpeed = 0;
    wchar_t speedInfo[16] = L"\0";
    while (currentProgress < 100) {
        EnterCriticalSection(&progressCriticalSection);
        if (downlodedTotalSize > 0) {
            double ratio = (double)downlodedTotalSize / (double)totalSize;
            currentProgress = (int)round(ratio * 100.0);

            SetProgressBarPosition(currentProgress);

            nSpeed = (downlodedTotalSize - prevDownloadedTotalSize) / 1024.0;
            if (nSpeed >= 1024.0) {
                nSpeed = nSpeed / 1024.0;
                swprintf_s(speedInfo, _countof(speedInfo), L" %.2fMB/s", nSpeed);
            }
            else {
                swprintf_s(speedInfo, _countof(speedInfo), L" %dKB/s", (int)nSpeed);
            }

            swprintf_s(progressIngMsg, sizeof(progressIngMsg) / sizeof(wchar_t), L"%ls Downloading: %d%% %ls",
                pSoftwareInfo->fileFullName, currentProgress, speedInfo);
            UpdateStaticLabelInfo(progressIngMsg);
        }

        prevDownloadedTotalSize = downlodedTotalSize;
        LeaveCriticalSection(&progressCriticalSection);

        Sleep(1000);
    }

    SetProgressBarPosition(0);
    UpdateStaticLabelInfo(L"");

    swprintf_s(progressFinishMsg, sizeof(progressFinishMsg) / sizeof(wchar_t), L"INFO: %ls Download: %i%%\r\n", pSoftwareInfo->fileFullName, currentProgress);
    AppendEditInfo(progressFinishMsg);

    Log("ProgressBar finish\r\n");
    return 0;
}

int compareFunc(const void* a, const void* b) {
    const wchar_t* strA = *(const wchar_t**)a;
    const wchar_t* strB = *(const wchar_t**)b;

    const wchar_t* partPosA = wcsstr(strA, L"part_");
    while (wcsstr(partPosA + 5, L"part_")) {
        partPosA = wcsstr(partPosA + 5, L"part_");
    }

    const wchar_t* partPosB = wcsstr(strB, L"part_");
    while (wcsstr(partPosB + 5, L"part_")) {
        partPosB = wcsstr(partPosB + 5, L"part_");
    }

    if (!partPosA || !partPosB) return wcscmp(strA, strB);

    unsigned long long numA = wcstoull(partPosA + 5, NULL, 10);
    unsigned long long numB = wcstoull(partPosB + 5, NULL, 10);

    if (numA < numB) return -1;
    if (numA > numB) return 1;
    return 0;
}

void freeGlobalParts(DownloadPart** parts, int size) {
    for (int i = 0; i < size; i++) {
        free((void*)parts[i]->version);
        free((void*)parts[i]->url);
        free((void*)parts[i]->filename);
        free((void*)parts[i]->filepath);

        free(parts[i]);
    }

    free(parts);
}

bool UnzipFile(SoftwareInfo* pSoftwareInfo) {

    wchar_t unzipZipFilePath[512] = { '\0' };
    wchar_t unzipTempMsg[512] = { '\0' };
    int unzipResult = 0;

    swprintf_s(unzipTempMsg, sizeof(unzipTempMsg) / sizeof(wchar_t), L"INFO: Unpacking %ls ...\r\n", pSoftwareInfo->fileFullName);
    AppendEditInfo(unzipTempMsg);

    swprintf_s(unzipZipFilePath, sizeof(unzipZipFilePath) / sizeof(wchar_t), L"%ls/%ls", DIRECTORY_DOWNLOAD, pSoftwareInfo->fileFullName);

    unzipResult = extract_zip_file(unzipZipFilePath, pSoftwareInfo->serviceType, pSoftwareInfo->version);
    if (unzipResult == -1) {
        Log(L"Failed to extract the file %ls, please try downloading again.", pSoftwareInfo->version);

        DeleteFile(unzipZipFilePath);

        return false;
    }
    else if (unzipResult == -3) {
        swprintf_s(unzipTempMsg, sizeof(unzipTempMsg) / sizeof(wchar_t),
            L"The corresponding version folder already exists: %ls/%ls/%ls\r\n", DIRECTORY_SERVICE, pSoftwareInfo->serviceType, pSoftwareInfo->version);
        AppendEditInfo(unzipTempMsg);

        return false;
    }
    else if (unzipResult == 0) {
        swprintf_s(unzipTempMsg, sizeof(unzipTempMsg) / sizeof(wchar_t), L"INFO: Unpacking %ls complete\r\n", pSoftwareInfo->fileFullName);
        AppendEditInfo(unzipTempMsg);

    }
    else {
        LogAndMsgBox(L"Unknown unzip error.\r\n");
        DeleteFile(unzipZipFilePath);

        return false;
    }

    if (wcscmp(pSoftwareInfo->serviceType, L"apache") == 0) {
        InitializeApacheConfigFile(*pSoftwareInfo);
    }

    if (wcscmp(pSoftwareInfo->serviceType, L"php") == 0) {
        syncPHPConfigFile(*pSoftwareInfo);
    }

    return true;
}

static CRITICAL_SECTION* lock_cs = nullptr;

void win32_locking_callback(int mode, int type, const char* file, int line) {
    if (!lock_cs) {
        lock_cs = static_cast<CRITICAL_SECTION*>(OPENSSL_malloc(CRYPTO_num_locks() * sizeof(CRITICAL_SECTION)));
        for (int i = 0; i < CRYPTO_num_locks(); i++)
            InitializeCriticalSection(&lock_cs[i]);
    }

    if (mode & CRYPTO_LOCK) {
        EnterCriticalSection(&lock_cs[type]);
    }
    else {
        LeaveCriticalSection(&lock_cs[type]);
    }
}

DWORD WINAPI DownloadManagerThread(LPVOID param) {
    SoftwareInfo* pSoftwareInfo = (SoftwareInfo*)param;

    int totalPartSize = 128;
    downlodedTotalSize = 0;
    numDynamicSubPartSize = 16;

    abnormalCloseThreadCount = 0;       // Count of threads exiting due to exception
    abnormalCount = 0;

    numDownloadThreadsSize = 0;
    bStartMonitor = false;

    numLockFlowMax = 16;

    if (!DirectoryExists(DIRECTORY_DOWNLOAD)) {
        CreateDirectory(DIRECTORY_DOWNLOAD, NULL);
    }

    // SSL library multithreading
    CRYPTO_set_locking_callback(win32_locking_callback);

    curl_global_init(CURL_GLOBAL_ALL);
    totalSize = CurlGetRemoteFileSize(pSoftwareInfo->link);
    curl_global_cleanup();

    if (totalSize < 1024) {
        LogAndMsgBox(L"Network exception,get remote file size error.");
        return 0;
    }

    Log("RemoteFile url: %ls\r\n", pSoftwareInfo->link);
    Log("RemoteFile totalSize: %llu bytes\r\n", totalSize);

    workerQueueArray = (Queue*)malloc(sizeof(Queue));
    if (!initializeQueue(workerQueueArray, 256)) {
        Log("Thread stack memory error.\r\n");
        return 0;
    }

    threadStackArray = (Stack*)malloc(sizeof(Stack));
    if (!initializeStack(threadStackArray, 64)) {
        Log("Thread stack memory error.\r\n");
        return 0;
    }

    ULONGLONG partSize = totalSize / totalPartSize;
    if (partSize > ULONG_MAX) {
        return 0;
    }

    globalPartGroup = (DownloadPart**)malloc(totalPartSize * sizeof(DownloadPart*));

    for (int i = 0; i < totalPartSize; i++) {
        unsigned long long start = i * partSize;
        unsigned long long end = (i == totalPartSize - 1) ? (totalSize - 1) : (start + partSize - 1);

        DownloadPart* part = GenerateDownloadPartModel(pSoftwareInfo->version, pSoftwareInfo->link, start, end);
        part->index = i;
        globalPartGroup[i] = part;

        if (CheckFileExists(part->filepath)) {
            unsigned long long fileSize = 0;
            GetFileSize(part->filepath, &fileSize);

            downlodedTotalSize += fileSize;
            part->readBytes = fileSize;
            part->currentStartByte = part->startByte + part->readBytes;

            if (part->totalBytesLength == fileSize) {   // complete
                part->status = 1;
            }
            else {                                      // incomplete
                enqueue(workerQueueArray, *part);
            }
        } else {                                        // new
            enqueue(workerQueueArray, *part);
        }
    }

    wchar_t startDownloadMsg[256];
    swprintf_s(startDownloadMsg, _countof(startDownloadMsg), L"INFO: Target link %ls\r\n", pSoftwareInfo->link);
    AppendEditInfo(startDownloadMsg);

    curl_global_init(CURL_GLOBAL_ALL);

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    HANDLE* threadsArray = NULL;
    HANDLE progressHandle = CreateThread(NULL, 0, ProgressThread, pSoftwareInfo, 0, NULL);
    HANDLE daemonMonitorHandle = CreateThread(NULL, 0, DaemonMonitorThread, NULL, 0, NULL);

    int threadNumberIndex = 0;
    threadManager = (ThreadManager**)malloc(sizeof(ThreadManager*));

    while (true) {
        EnterCriticalSection(&progressCriticalSection);

        if (
            numDownloadThreadsSize == MAX_THREAD_NUM
            || abnormalCloseThreadCount > 0 // if the process is terminated, it means that the process can no longer be created.
            ) {
            LeaveCriticalSection(&progressCriticalSection);
            break;
        }

        numDownloadThreadsSize += 1;

        threadsArray = (HANDLE*)realloc(threadsArray, numDownloadThreadsSize * sizeof(HANDLE));
        if (threadsArray == NULL) {
            free(threadsArray);
            LogAndMsgBox("Thread malloc memory failed threadsArray\r\n");
            LeaveCriticalSection(&progressCriticalSection);
            break;
        }

        int* threadIndex = (int*)malloc(sizeof(int));
        if (threadIndex == NULL) {
            LogAndMsgBox("Thread malloc memory failed threadIndex\r\n");
            LeaveCriticalSection(&progressCriticalSection);
            break;
        }

        *threadIndex = threadNumberIndex;

        threadManager = (ThreadManager**)realloc(threadManager, numDownloadThreadsSize * sizeof(ThreadManager*));
        if (threadManager == NULL) {
            free(threadManager);
            LogAndMsgBox("Thread malloc memory failed threadManager\r\n");
            LeaveCriticalSection(&progressCriticalSection);
            break;
        }

        ThreadManager tempManager;
        tempManager.status = 0;
        tempManager.threadIndex = *threadIndex;
        //tempManager.realPartSize = -1;

        threadManager[*threadIndex] = &tempManager;

        stackPush(threadStackArray, threadManager[*threadIndex]);

        LeaveCriticalSection(&progressCriticalSection);

        Log("Create thread index:%d; status:%d;\r\n",
            threadManager[*threadIndex]->threadIndex, threadManager[*threadIndex]->status);

        threadsArray[threadNumberIndex] = CreateThread(NULL, 0, DaemonDownloadThread, threadIndex, 0, NULL);

        // Infer whether the next thread can be created normally based on the current thread's request

        threadNumberIndex += 1;
    }

    bStartMonitor = true;

    int waitExecResult = WaitForMultipleObjects(numDownloadThreadsSize, threadsArray, TRUE, INFINITE);

    if (progressHandle) {
        WaitForSingleObject(progressHandle, INFINITE);
        CloseHandle(progressHandle);
    }

    if (daemonMonitorHandle) {
        WaitForSingleObject(daemonMonitorHandle, INFINITE);
        CloseHandle(daemonMonitorHandle);
    }

    if (hEvent) {
        CloseHandle(hEvent);
    }

    curl_global_cleanup();

    const wchar_t** filepathPtrs = new const wchar_t* [totalPartSize]();

    // Check the integrity of the file block.
    bool checkVerify = true;
    for (int i = 0; i < totalPartSize; i++) {
        if (globalPartGroup[i]->totalBytesLength != globalPartGroup[i]->readBytes) {
            Log("ERROR: successfulDownloads___readBytes:%llu__________totalBytesLength:%llu\r\n",
                globalPartGroup[i]->readBytes, globalPartGroup[i]->totalBytesLength);

            for (int j = 0; j < totalPartSize; j++) {
                DeleteFile(globalPartGroup[i]->filepath);
            }

            checkVerify = false;
            break;
        }

        ULONGLONG fileSizeValue = 0;
        GetFileSize(globalPartGroup[i]->filepath, &fileSizeValue);
        if (fileSizeValue != globalPartGroup[i]->totalBytesLength) {
            Log("file size error______readBytes:%llu;____totalBytes%llu;______:%ls \r\n\r\n",
                globalPartGroup[i]->readBytes,
                globalPartGroup[i]->totalBytesLength,
                globalPartGroup[i]->filename);
        }

        filepathPtrs[i] = globalPartGroup[i]->filepath;
    }

    qsort(filepathPtrs, totalPartSize, sizeof(wchar_t*), compareFunc);
    for (int i = 0; i < totalPartSize; i++) {
        Log("filename:%ls;\r\n",
            filepathPtrs[i]);
    }

    if (checkVerify) {
        wchar_t fileFullNamePath[256];
        swprintf_s(fileFullNamePath, sizeof(fileFullNamePath) / sizeof(wchar_t), L"%ls/%ls", DIRECTORY_DOWNLOAD, pSoftwareInfo->fileFullName);

        MergeFiles(fileFullNamePath, filepathPtrs, totalPartSize);
    }
    else {
        Log("File checkVerify error\r\n");
    }

    DWORD exitCode;
    for (int i = 0; i < numDownloadThreadsSize; i++) {
        if (threadsArray[i]) {
            if (GetExitCodeThread(threadsArray[i], &exitCode)) {
                if (exitCode == STILL_ACTIVE) {
                    CloseHandle(threadsArray[i]);
                }
            }
        }
    }

    if (UnzipFile(pSoftwareInfo)) {
    }

    free(threadsArray);
    delete[] filepathPtrs;

    freeGlobalParts(globalPartGroup, totalPartSize);
    freeQueue(workerQueueArray);

    free(threadManager);
    freeStack(threadStackArray);
    free(threadStackArray);

    return 1;
}

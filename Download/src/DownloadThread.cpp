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

typedef struct {
    DownloadPart* parts;
    int size;
    int capacity;
} DynamicArray;

unsigned long long totalSize;
unsigned long long downlodedTotalSize = 0;

int numDownloadThreadsSize = 0;
int numDynamicSubPartSize = 0;
int abnormalCount = 0;
int abnormalCloseThreadCount = 0;
bool bStartMonitor = false;

DynamicArray successfulDownloads;
Queue* workerQueueArray;
CircularQueue circularQueueArray;
Stack* threadStackArray;

ThreadManager** threadManager;

void initDynamicArray(DynamicArray* arr, int initialSize) {
    arr->parts = (DownloadPart*)malloc(initialSize * sizeof(DownloadPart));
    if (!arr->parts) {
        Log("Memory allocation failed.\r\n");
        exit(1);
    }

    arr->size = 0;
    arr->capacity = initialSize;
}

void deepCopyPart(DownloadPart* dest, const DownloadPart* src) {
    dest->indexCategory = src->indexCategory;

    dest->startByte = src->startByte;
    dest->endByte = src->endByte;
    dest->totalBytesLength = src->totalBytesLength;
    dest->readBytes = src->readBytes;
    dest->status = src->status;

    dest->version = _wcsdup(src->version);
    dest->url = _wcsdup(src->url);

    dest->filename = _wcsdup(src->filename);
    dest->filepath = _wcsdup(src->filepath);
}

void PushAndDeepDynamicArray(DynamicArray* arr, DownloadPart* part) {
    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->parts = (DownloadPart*)realloc(arr->parts, arr->capacity * sizeof(DownloadPart));
        if (!arr->parts) {
            // Memory allocation failed.
            Log("Memory allocation failed.\r\n");
            return;
        }
    }

    deepCopyPart(&arr->parts[arr->size], part);

    arr->size++;
}

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
    part->indexCategory = 0;
    part->startByte = start;
    part->endByte = end;
    part->readBytes = readBytes;
    part->currentStartByte = part->startByte + part->readBytes;
    part->totalBytesLength = part->endByte - part->startByte + 1;
    part->status = 0;

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

        if (bStartMonitor == true) {    // Start monitor thread state
            EnterCriticalSection(&progressCriticalSection);
            if (abnormalCloseThreadCount < (numDownloadThreadsSize - 2)) {
                if (abnormalCount > 3) {    // Reduce thread num
                    ThreadManager* subThreadManager;
                    if (stackPop(threadStackArray, &subThreadManager)) {
                        abnormalCloseThreadCount += 1;
                        threadManager[subThreadManager->threadIndex]->status = -1;

                        abnormalCount = 0;
                        Log("AbnormalCount size change;\r\n");
                    }
                }
            }
            else {
                LeaveCriticalSection(&progressCriticalSection);
                break;
            }
            LeaveCriticalSection(&progressCriticalSection);
        }
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
        //DownloadPart* partGroup = new DownloadPart[numDynamicSubPartSize]();
        DownloadPart** partGroup = (DownloadPart**)malloc(sizeof(DownloadPart*));

        // if createThread abnormal > 5 then Sleep(1000); createThread

        while (counterSizeOrIndex < numDynamicSubPartSize) {
            EnterCriticalSection(&progressCriticalSection);
            DownloadPart* part;
            if (dequeue(workerQueueArray, &part)) {

                int tempSize = (counterSizeOrIndex + 1);
                partGroup = (DownloadPart**)realloc(partGroup, tempSize * sizeof(DownloadPart*));
                part->timestamp = GetTickCount64();
                partGroup[counterSizeOrIndex] = part;

                counterSizeOrIndex += 1;

                LeaveCriticalSection(&progressCriticalSection);
            }
            else {
                LeaveCriticalSection(&progressCriticalSection);
                break;
            }
        }

        if (counterSizeOrIndex > 0) {
            CurlMultipleDownloadThread(partGroup, counterSizeOrIndex);

            for (int i = 0; i < counterSizeOrIndex; i++) {
                EnterCriticalSection(&progressCriticalSection);
                PushAndDeepDynamicArray(&successfulDownloads, partGroup[i]);
                LeaveCriticalSection(&progressCriticalSection);
            }

            for (int i = 0; i < counterSizeOrIndex; i++) {
                freeDownloadPart(partGroup[i]);
                free(partGroup[i]);
            }
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

void freeDynamicArray(DynamicArray* arr) {
    for (int i = 0; i < arr->size; i++) {
        free((void*)arr->parts[i].version);
        free((void*)arr->parts[i].url);
        free((void*)arr->parts[i].filename);
        free((void*)arr->parts[i].filepath);
    }

    free(arr->parts);

    arr->size = 0;
    arr->capacity = 0;
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
    numDynamicSubPartSize = 6;

    abnormalCloseThreadCount = 0;       // Count of threads exiting due to exception
    abnormalCount = 0;

    numDownloadThreadsSize = 0;
    bStartMonitor = false;

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

    initDynamicArray(&successfulDownloads, 1);

    ULONGLONG partSize = totalSize / totalPartSize;
    if (partSize > ULONG_MAX) {
        return 0;
    }

    for (int i = 0; i < totalPartSize; i++) {
        unsigned long long start = i * partSize;
        unsigned long long end = (i == totalPartSize - 1) ? (totalSize - 1) : (start + partSize - 1);

        DownloadPart* part = GenerateDownloadPartModel(pSoftwareInfo->version, pSoftwareInfo->link, start, end);

        if (CheckFileExists(part->filepath)) {
            unsigned long long fileSize = 0;
            GetFileSize(part->filepath, &fileSize);

            downlodedTotalSize += fileSize;
            part->readBytes = fileSize;
            part->currentStartByte = part->startByte + part->readBytes;

            if (part->totalBytesLength == fileSize) {   // complete
                PushAndDeepDynamicArray(&successfulDownloads, part);

                freeDownloadPart(part);
                free(part);
            }
            else {                                      // incomplete
                enqueue(workerQueueArray, part);
            }
        } else {                                        // new
            enqueue(workerQueueArray, part);
        }
    }

    wchar_t startDownloadMsg[256];
    swprintf_s(startDownloadMsg, _countof(startDownloadMsg), L"INFO: Target link %ls\r\n", pSoftwareInfo->link);
    AppendEditInfo(startDownloadMsg);

    curl_global_init(CURL_GLOBAL_ALL);

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

        threadManager[*threadIndex] = &tempManager;

        stackPush(threadStackArray, threadManager[*threadIndex]);

        LeaveCriticalSection(&progressCriticalSection);

        Log("Create thread index:%d; status:%d;\r\n",
            threadManager[*threadIndex]->threadIndex, threadManager[*threadIndex]->status);

        threadsArray[threadNumberIndex] = CreateThread(NULL, 0, DaemonDownloadThread, threadIndex, 0, NULL);

        threadNumberIndex += 1;

        Sleep(1500);
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

    curl_global_cleanup();

    //Log("Request_finish: %d\r\n", waitExecResult);

    const wchar_t** filepathPtrs = new const wchar_t* [successfulDownloads.size]();

    // Check the integrity of the file block.
    bool checkVerify = true;
    for (int i = 0; i < successfulDownloads.size; i++) {
        if (successfulDownloads.parts[i].totalBytesLength != successfulDownloads.parts[i].readBytes) {
            Log("ERROR: successfulDownloads___readBytes:%llu__________totalBytesLength:%llu\r\n",
                successfulDownloads.parts[i].readBytes, successfulDownloads.parts[i].totalBytesLength);

            for (int j = 0; j < successfulDownloads.size; j++) {
                DeleteFile(successfulDownloads.parts[j].filepath);
            }

            checkVerify = false;
            break;
        }

        ULONGLONG fileSizeValue = 0;
        GetFileSize(successfulDownloads.parts[i].filepath, &fileSizeValue);
        if (fileSizeValue != successfulDownloads.parts[i].totalBytesLength) {
            Log("file size error______readBytes:%llu;____totalBytes%llu;______:%ls \r\n\r\n",
                successfulDownloads.parts[i].readBytes,
                successfulDownloads.parts[i].totalBytesLength,
                successfulDownloads.parts[i].filename);
        }

        filepathPtrs[i] = successfulDownloads.parts[i].filepath;
    }

    qsort(filepathPtrs, successfulDownloads.size, sizeof(wchar_t*), compareFunc);
    for (int i = 0; i < successfulDownloads.size; i++) {
        Log("filename:%ls;\r\n",
            filepathPtrs[i]);
    }

    if (checkVerify) {
        wchar_t fileFullNamePath[256];
        swprintf_s(fileFullNamePath, sizeof(fileFullNamePath) / sizeof(wchar_t), L"%ls/%ls", DIRECTORY_DOWNLOAD, pSoftwareInfo->fileFullName);

        MergeFiles(fileFullNamePath, filepathPtrs, successfulDownloads.size);
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

    freeDynamicArray(&successfulDownloads);
    freeQueue(workerQueueArray);

    freeCircularQueue(&circularQueueArray);

    free(threadManager);
    freeStack(threadStackArray);
    free(threadStackArray);

    return 1;
}

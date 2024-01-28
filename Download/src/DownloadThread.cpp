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
int numThreadDispathPartSize = 0;
int abnormalCount = 0;
int abnormalCloseThreadCount = 0;
int numLockFlow = 0;
int numLockFlowMax = 0;

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
    const char* version,
    const char* link,
    ULONGLONG start,
    ULONGLONG end,
    const char* fileName = "\0",
    const char* filePath = "\0",
    ULONGLONG readBytes = 0
) {

    DownloadPart* part = (DownloadPart*)malloc(sizeof(DownloadPart));

    if (fileName[0] == L'\0') {
        char TempFilename[256] = { '\0' };
        sprintf_s(TempFilename, sizeof(TempFilename), "%s_part_%llu_%llu", version, start, end);
        part->filename = _strdup(TempFilename);
    }
    else {
        part->filename = _strdup(fileName);
    }

    if (filePath[0] == L'\0') {
        char TempFilePath[256] = { '\0' };
        sprintf_s(TempFilePath, sizeof(TempFilePath), "%s/%s", DIRECTORY_DOWNLOAD, part->filename);
        part->filepath = _strdup(TempFilePath);
    }
    else {
        part->filepath = _strdup(filePath);
    }

    part->version = _strdup(version);
    part->url = _strdup(link);
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
            if (abnormalCount > 0) {
                numLockFlowMax -= abnormalCount * 2;

                abnormalCount = 0;
            }
            else {
                numLockFlowMax += 2;    // only addition
            }

            if (numLockFlowMax < 8) numLockFlowMax = 8;
            if (numLockFlowMax > 32) numLockFlowMax = 32;

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
        if (isDone()) {
            Log("DaemonDownloadThread exit index: %d\r\n", threadIndex);
            break;
        }

        int counterSizeOrIndex = 0;
        DownloadPart** partGroup = (DownloadPart**)malloc(sizeof(DownloadPart*));

        EnterCriticalSection(&progressCriticalSection);
        while (counterSizeOrIndex < numThreadDispathPartSize) {
            if (numLockFlow >= numLockFlowMax) {
                break;
            }

            DownloadPart tempPart;
            if (dequeue(workerQueueArray, &tempPart)) {
                int tempSize = (counterSizeOrIndex + 1);
                partGroup = (DownloadPart**)realloc(partGroup, tempSize * sizeof(DownloadPart*));

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
                    partGroup[i]->statusCode = -1;
                    
                    enqueue(workerQueueArray, *partGroup[i]);
                    LeaveCriticalSection(&progressCriticalSection);
                }
            }
        }

        free(partGroup);

        Sleep(100);
    }

    free(param);

    return 0;
}

DWORD WINAPI ProgressThread(LPVOID param) {
    SoftwareInfo* pSoftwareInfo = (SoftwareInfo*)param;

    char progressIngMsg[512];
    char progressFinishMsg[512];

    // init
    SetProgressBarPosition(0);
    UpdateStaticLabelInfo("");

    int currentProgress = 0;
    unsigned long long prevDownloadedTotalSize = 0;
    double nSpeed = 0;
    char speedInfo[16] = "\0";
    while (currentProgress < 100) {
        EnterCriticalSection(&progressCriticalSection);
        if (downlodedTotalSize > 0) {
            double ratio = (double)downlodedTotalSize / (double)totalSize;
            currentProgress = (int)round(ratio * 100.0);

            SetProgressBarPosition(currentProgress);

            nSpeed = (downlodedTotalSize - prevDownloadedTotalSize) / 1024.0;
            if (nSpeed >= 1024.0) {
                nSpeed = nSpeed / 1024.0;
                sprintf_s(speedInfo, sizeof(speedInfo), " %.2fMB/s", nSpeed);
            }
            else {
                sprintf_s(speedInfo, sizeof(speedInfo), " %dKB/s", (int)nSpeed);
            }

            sprintf_s(progressIngMsg, sizeof(progressIngMsg), "%s Downloading: %d%% %s",
                pSoftwareInfo->fileFullName, currentProgress, speedInfo);
            UpdateStaticLabelInfo(progressIngMsg);
        }

        prevDownloadedTotalSize = downlodedTotalSize;
        LeaveCriticalSection(&progressCriticalSection);

        Sleep(500);
    }

    SetProgressBarPosition(0);
    UpdateStaticLabelInfo("");

    sprintf_s(progressFinishMsg, sizeof(progressFinishMsg), "INFO: %s Download: %i%%\r\n", pSoftwareInfo->fileFullName, currentProgress);
    AppendEditInfo(progressFinishMsg);

    Log("ProgressBar finish\r\n");
    return 0;
}

int compareFunc(const void* a, const void* b) {
    const char* strA = *(const char**)a;
    const char* strB = *(const char**)b;

    const char* partPosA = strstr(strA, "part_");
    while (strstr(partPosA + 5, "part_")) {
        partPosA = strstr(partPosA + 5, "part_");
    }

    const char* partPosB = strstr(strB, "part_");
    while (strstr(partPosB + 5, "part_")) {
        partPosB = strstr(partPosB + 5, "part_");
    }

    if (!partPosA || !partPosB) return strcmp(strA, strB);

    unsigned long long numA = strtoull(partPosA + 5, NULL, 10);
    unsigned long long numB = strtoull(partPosB + 5, NULL, 10);

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

    char unzipZipFilePath[512] = { '\0' };
    char unzipTempMsg[512] = { '\0' };
    int unzipResult = 0;

    sprintf_s(unzipTempMsg, sizeof(unzipTempMsg), "INFO: Unpacking %s ...\r\n", pSoftwareInfo->fileFullName);
    AppendEditInfo(unzipTempMsg);

    sprintf_s(unzipZipFilePath, sizeof(unzipZipFilePath), "%s/%s", DIRECTORY_DOWNLOAD, pSoftwareInfo->fileFullName);

    unzipResult = extract_zip_file(unzipZipFilePath, pSoftwareInfo->serviceType, pSoftwareInfo->version);
    if (unzipResult == -1) {
        Log(L"Failed to extract the file %s, please try downloading again.", pSoftwareInfo->version);

        DeleteFileA(unzipZipFilePath);

        return false;
    }
    else if (unzipResult == -3) {
        sprintf_s(unzipTempMsg, sizeof(unzipTempMsg),
            "The corresponding version folder already exists: %s/%s/%s\r\n", DIRECTORY_SERVICE, pSoftwareInfo->serviceType, pSoftwareInfo->version);
        AppendEditInfo(unzipTempMsg);

        return false;
    }
    else if (unzipResult == 0) {
        sprintf_s(unzipTempMsg, sizeof(unzipTempMsg), "INFO: Unpacking %s complete\r\n", pSoftwareInfo->fileFullName);
        AppendEditInfo(unzipTempMsg);

    }
    else {
        LogAndMsgBox("Unknown unzip error.\r\n");
        DeleteFileA(unzipZipFilePath);

        return false;
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

    int totalPartSize = 32;
    downlodedTotalSize = 0;
    numThreadDispathPartSize = 2;

    //abnormalCloseThreadCount = 0;       // Count of threads exiting due to exception
    abnormalCount = 0;

    numDownloadThreadsSize = 0;
    bStartMonitor = false;

    numLockFlow = 0;
    numLockFlowMax = 32;

    if (!DirectoryExists(DIRECTORY_DOWNLOAD)) {
        CreateDirectoryA(DIRECTORY_DOWNLOAD, NULL);
    }

    // SSL library multithreading
    CRYPTO_set_locking_callback(win32_locking_callback);

    // Starting download soon

    //AppendEditInfo(L"Starting download soon");
    char infoTip[256] = "\0";
    sprintf_s(infoTip, sizeof(infoTip), "INFO: Initiating download of %s\r\n", pSoftwareInfo->version);
    AppendEditInfo(infoTip);
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

    char startDownloadMsg[256];
    sprintf_s(startDownloadMsg, sizeof(startDownloadMsg), "INFO: Target link %s\r\n", pSoftwareInfo->link);
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
            //|| abnormalCloseThreadCount > 0 // if the process is terminated, it means that the process can no longer be created.
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

    const char** filepathPtrs = new const char* [totalPartSize]();

    // Check the integrity of the file block.
    bool checkVerify = true;
    for (int i = 0; i < totalPartSize; i++) {
        if (globalPartGroup[i]->totalBytesLength != globalPartGroup[i]->readBytes) {
            Log("ERROR: successfulDownloads___readBytes:%llu__________totalBytesLength:%llu\r\n",
                globalPartGroup[i]->readBytes, globalPartGroup[i]->totalBytesLength);

            for (int j = 0; j < totalPartSize; j++) {
                DeleteFileA(globalPartGroup[i]->filepath);
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

    qsort(filepathPtrs, totalPartSize, sizeof(char*), compareFunc);
    for (int i = 0; i < totalPartSize; i++) {
        Log("filename:%s;\r\n",
            filepathPtrs[i]);
    }

    if (checkVerify) {
        char fileFullNamePath[256];
        sprintf_s(fileFullNamePath, sizeof(fileFullNamePath), "%s/%s", DIRECTORY_DOWNLOAD, pSoftwareInfo->fileFullName);

        if (MergeFiles(fileFullNamePath, filepathPtrs, totalPartSize) == 0) {
            UnzipFile(pSoftwareInfo);
        }
        else {
            MessageBoxA(NULL, "Failed to merge and decompress files after downloading", NULL, 0);
        }
    }
    else {
        Log("File checkVerify error\r\n");
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

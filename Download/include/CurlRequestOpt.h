#pragma once

#include "DownloadQueue.h"

#define NUM_SUBPARTS 8

extern CRITICAL_SECTION progressCriticalSection;

extern unsigned long long downlodedTotalSize;

extern CRITICAL_SECTION* targetLocks;

extern int numDownloadThreads;

extern Queue* workerQueueArray;

extern int abnormalCount;

extern int numDynamicSubPartSize;

extern int numLockFlow;

extern HANDLE hEvent;

extern int numLockFlowMax;

unsigned long long CurlGetRemoteFileSize(const char* url);

DWORD CurlMultipleDownloadThread(LPVOID param, int numDynamicSubPartSize);


#pragma once

#include "ServiceSource.h"
#include "curl/curl.h"

struct DownloadPart {
    const wchar_t* version;
    const wchar_t* url;
    const wchar_t* filename;
    const wchar_t* filepath;
    FILE* file;
    CURL* easy_handle;

    unsigned long index;
    //unsigned long retryCount;
    //unsigned long abnormalCount;

    unsigned long long startByte;
    unsigned long long endByte;
    unsigned long long currentStartByte;

    unsigned long long readBytes;
    unsigned long long totalBytesLength;

    unsigned int status;
    int statusCode;
    unsigned long long timestamp;
};

typedef struct {
    int threadIndex;
    int status;
} ThreadManager;

extern HWND hWndMain;
extern CRITICAL_SECTION progressCriticalSection;

DWORD WINAPI DownloadManagerThread(LPVOID param);

bool UnzipFile(SoftwareInfo* pSoftwareInfo);
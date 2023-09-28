#include "framework.h"
#include <stdarg.h>
#include <psapi.h>
#include "Common.h"

bool WToM(const wchar_t* wideStr, char* outputBuffer, int bufferSize) {
    if (!wideStr || !outputBuffer || bufferSize <= 0) return false;

    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (requiredSize == 0 || requiredSize > bufferSize) {
        return false;
    }

    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, outputBuffer, bufferSize, NULL, NULL);

    return true;
}

bool MToW(const char* multiByteStr, wchar_t* outputBuffer, int bufferSize) {
    if (!multiByteStr || !outputBuffer || bufferSize <= 0) return false;

    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, multiByteStr, -1, NULL, 0);
    if (requiredSize == 0 || requiredSize > bufferSize) {
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, multiByteStr, -1, outputBuffer, bufferSize);

    return true;
}

void InitRandomSeed() {
    srand((unsigned int)GetTickCount64());
}

int RandomInRange(int min, int max) {
    return min + rand() % (max + 1 - min);
}

SIZE_T GetCurrentProcessMemoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX memInfo;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&memInfo, sizeof(memInfo));
    return memInfo.WorkingSetSize;
}


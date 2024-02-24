#include "framework.h"
#include <stdarg.h>
#include <psapi.h>
#include "Common.h"
#include "stdio.h"

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

BOOL Is64BitWindows() {
#if defined(_WIN64)
    // If compiling as a 64-bit application, the operating system must be 64-bit
    return TRUE; // 64-bit program
#elif defined(_WIN32)
    // If compiled as a 32-bit application, you need to check if it is running on 64-bit Windows
    BOOL f64 = FALSE;
    if (!IsWow64Process(GetCurrentProcess(), &f64)) {
        // If IsWow64Process fails, we can assume 32-bit Windows
        return FALSE;
    }
    return f64; // If f64 is TRUE, then 64-bit Windows
#else
    // Undefined, assumed to be unsupported platforms
    return FALSE;
#endif
}

BOOL IsWindows8OrGreater() {
    OSVERSIONINFOEX osvi;
    DWORDLONG dwlConditionMask = 0;
    int op = VER_GREATER_EQUAL;

    // Initializing the OSVERSIONINFOEX Structure
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 6;
    osvi.dwMinorVersion = 2; // The version number for Windows 8 is 6.2

    // Initialization Condition Mask
    VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
    VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);

    // Check if the current Windows version meets the conditions
    return VerifyVersionInfo(
        &osvi,
        VER_MAJORVERSION | VER_MINORVERSION,
        dwlConditionMask
    );
}

BOOL CheckVersionSystemEnable(const char* str) {
    if (str == NULL || strlen(str) < 3) {
        return FALSE;
    }

    BOOL isX64 = Is64BitWindows();

    size_t len = strlen(str);

    char lastThree[4];
    strncpy_s(lastThree, str + len - 3, 3);
    lastThree[3] = '\0';

    if (!isX64) {
        if (strcmp(lastThree, "x64") == 0) {
            return FALSE;
        }
    }

    return TRUE;
}

int extractVersionNumber(const char* prefix, const char* str) {
    int version = -1;

    const char* vcPos = strstr(str, prefix);
    if (vcPos != NULL) {
        if (sscanf_s(vcPos, "vc%d", &version) != 1) {
            version = -1;
        }
    }

    return version;
}
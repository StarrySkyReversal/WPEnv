#include "framework.h"
#include <tlhelp32.h>
#include <stdbool.h>
#include <wchar.h>
#include "ModeMonitor.h"
#include "Log.h"

// A function to compare two wide strings, used for qsort.
int wcompare(const void* a, const void* b) {
    return wcscmp(*(wchar_t**)a, *(wchar_t**)b);
}

// A simple hash function for wide strings.
DWORD simpleHash(wchar_t* names[], size_t count, FILETIME* times, size_t timesCount) {
    DWORD hash = 0;
    for (size_t i = 0; i < count; i++) {
        for (wchar_t* c = names[i]; *c; c++) {
            hash ^= *c;
        }
    }
    for (size_t i = 0; i < timesCount; i++) {
        hash ^= times[i].dwLowDateTime;
        hash ^= times[i].dwHighDateTime;
    }
    return hash;
}

DWORD getParentProcessIdForName(DWORD processId, wchar_t* parentName, DWORD bufferSize) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);

    if (Process32FirstW(hSnapShot, &pEntry)) {
        do {
            if (pEntry.th32ProcessID == processId) {
                wcsncpy_s(parentName, bufferSize, pEntry.szExeFile, _TRUNCATE);

                CloseHandle(hSnapShot);
                return pEntry.th32ParentProcessID;
            }
        } while (Process32NextW(hSnapShot, &pEntry));
    }

    CloseHandle(hSnapShot);

    return 0; // Not found
}

DWORD getTargetProcessesHash(const wchar_t* targetProcesses[], size_t targetCount) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    DWORD hash = 0; // Initialize hash value

    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);
    if (Process32FirstW(hSnapShot, &pEntry)) {
        do {
            for (size_t i = 0; i < targetCount; i++) {
                if (wcscmp(pEntry.szExeFile, targetProcesses[i]) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pEntry.th32ProcessID);

                    HANDLE hParentProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pEntry.th32ParentProcessID);
                    if (hProcess) {
                        FILETIME creationTime, exitTime, kernelTime, userTime;
                        if (GetProcessTimes(hParentProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
                            DWORD parentId = pEntry.th32ParentProcessID; // Get parent process ID

                            wchar_t parentProcessName[256];
                            getParentProcessIdForName(pEntry.th32ParentProcessID, parentProcessName, _countof(parentProcessName));

                            // find parentProcessName not equal processname
                            if (wcscmp(parentProcessName, targetProcesses[0]) != 0) {
                                //Log("create_time: %lu ______parent_id: %lu____parentName: %ls_____  process_id: %lu \r\n", creationTime.dwLowDateTime, parentId, parentProcessName, pEntry.th32ProcessID);

                                // Generate hash based on parent's creation time + parent's ProcessId + target's ProcessName
                                hash ^= parentId; // XOR operation to update hash
                                hash ^= (DWORD)creationTime.dwLowDateTime; // XOR with creation time low part
                                hash ^= (DWORD)creationTime.dwHighDateTime; // XOR with creation time high part

                                // XOR with process name, character by character
                                wchar_t* processName = pEntry.szExeFile;
                                while (*processName) {
                                    hash ^= (DWORD)(*processName);
                                    processName++;
                                }
                            }

                        }
                    }

                    if (hProcess) CloseHandle(hProcess);
                    if (hParentProcess) CloseHandle(hParentProcess);

                    break;
                }
            }
        } while (Process32NextW(hSnapShot, &pEntry));
    }
    CloseHandle(hSnapShot);

    return hash;
}

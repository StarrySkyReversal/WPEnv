#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <tlhelp32.h>
#include "Log.h"

BOOL ProcessIsRunning(const char* processName) {
    BOOL exists = FALSE;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            size_t convertedChars = 0;
            char buffer[512];
            wcstombs_s(&convertedChars, buffer, sizeof(buffer), entry.szExeFile, _TRUNCATE);
            if (!_stricmp(buffer, processName)) {
                exists = TRUE;
                break;
            }
        }
    }

    CloseHandle(snapshot);
    return exists;
}

bool IsHttpdParentRunning(const char* processName) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);

    bool isParent = false;
    bool exitLoop = false;  // New flag to control the outer loop

    BOOL hRes = Process32First(hSnapShot, &pEntry);
    while (hRes && !exitLoop) {  // Added check for exitLoop flag here
        if (strcmp((LPSTR)pEntry.szExeFile, processName) == 0) {
            DWORD parentId = pEntry.th32ParentProcessID;

            //Log("%ls _______ processId : %lu\r\n", processName, pEntry.th32ProcessID);
            //Log("%ls _______ parentId : %lu\r\n", processName, parentId);

            // Reset snapshot to the beginning.
            Process32FirstW(hSnapShot, &pEntry);

            while (Process32NextW(hSnapShot, &pEntry)) {
                if (pEntry.th32ProcessID == parentId) {
                    size_t convertedChars = 0;
                    char buffer[512];
                    wcstombs_s(&convertedChars, buffer, sizeof(buffer), pEntry.szExeFile, _TRUNCATE);

                    if (strcmp(buffer, processName) != 0) {
                        // The parent's name is different from httpd.exe, so the current httpd.exe is a parent process.
                        isParent = true;
                        exitLoop = true;  // Set the flag to exit the outer loop
                        break;  // Break out of the inner loop
                    }
                }
            }
        }
        if (!exitLoop) {  // Only proceed to the next item if we're not exiting the loop
            hRes = Process32NextW(hSnapShot, &pEntry);
        }
    }

    CloseHandle(hSnapShot);

    return isParent;
}

// Determine if the current process was created by CreateProcess
DWORD isSelfChildProcessOfCurrent(const char* processName) {
    DWORD currentProcessId = GetCurrentProcessId();
    int isChild = 0;

    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(pEntry);

    BOOL hRes = Process32First(hSnapShot, &pEntry);

    BOOL findTag = false;
    while (hRes) {
        if (strcmp((LPSTR)pEntry.szExeFile, processName) == 0) {
            // Found the process.
            findTag = true;

            if (pEntry.th32ParentProcessID == currentProcessId) {
                // It's a child process.
                isChild = 1;
            }
            else {
                //It's not a child process.
                isChild = 2;
            }

            break;
        }
        hRes = Process32NextW(hSnapShot, &pEntry);
    }
    CloseHandle(hSnapShot);

    if (findTag == false) {
        // Didn't find anything.
        isChild = -1;
    }

    return isChild;
}

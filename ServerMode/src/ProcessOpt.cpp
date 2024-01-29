#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <tlhelp32.h>
#include "Log.h"
#include "Common.h"

BOOL ProcessIsRunning(const char* processName) {
    BOOL exists = FALSE;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    char tempStr[512];

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {

            WToM(entry.szExeFile, tempStr, sizeof(tempStr));
            if (!_stricmp(tempStr, processName)) {
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

    char tempStr[512];

    while (hRes && !exitLoop) {  // Added check for exitLoop flag here
        WToM(pEntry.szExeFile, tempStr, sizeof(tempStr));
        if (strcmp(tempStr, processName) == 0) {
            DWORD parentId = pEntry.th32ParentProcessID;

            //Log("%ls _______ processId : %lu\r\n", processName, pEntry.th32ProcessID);
            //Log("%ls _______ parentId : %lu\r\n", processName, parentId);

            // Reset snapshot to the beginning.
            Process32FirstW(hSnapShot, &pEntry);

            while (Process32NextW(hSnapShot, &pEntry)) {
                if (pEntry.th32ProcessID == parentId) {
                    WToM(pEntry.szExeFile, tempStr, sizeof(tempStr));

                    if (strcmp(tempStr, processName) != 0) {
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

    char tempFileStr[512];

    while (hRes) {
        WToM(pEntry.szExeFile, tempFileStr, sizeof(tempFileStr));

        if (strcmp(tempFileStr, processName) == 0) {
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

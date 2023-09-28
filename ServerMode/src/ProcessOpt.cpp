#include <windows.h>
#include <wbemidl.h>
#include <comdef.h>
#include <tlhelp32.h>
#include "Log.h"

#pragma comment(lib, "wbemuuid.lib")

bool IsHttpdMainProcessRunning(const wchar_t* processName) {
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;

    CoInitialize(NULL);

    HRESULT hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr)) {
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t(L"SELECT ParentProcessId, ProcessId, CommandLine FROM Win32_Process WHERE Name='httpd.exe'"), WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator);
    if (FAILED(hr)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    ULONGLONG uParentId = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtParentProcessIdProp;
        //VARIANT vtProcessIdProp;
        //hr = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);

        hr = pclsObj->Get(L"ParentProcessId", 0, &vtParentProcessIdProp, 0, 0);  // Get the ProcessId property
        //hr = pclsObj->Get(L"ProcessId", 0, &vtProcessIdProp, 0, 0);  // Get the ProcessId property

        uParentId = vtParentProcessIdProp.ullVal;

        //wprintf(L"ParentProcessId: %llu\n", vtParentProcessIdProp.ullVal);
        //wprintf(L"ProcessId: %llu\n", vtProcessIdProp.ullVal);

        VariantClear(&vtParentProcessIdProp);
        pclsObj->Release();
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();

    CoUninitialize();

    return true;
}

BOOL ProcessIsRunning(LPCWSTR processName) {
    BOOL exists = FALSE;
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (!_wcsicmp(entry.szExeFile, processName)) {
                exists = TRUE;
                break;
            }
        }
    }

    CloseHandle(snapshot);
    return exists;
}

bool IsHttpdParentRunning(const wchar_t* processName) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pEntry;
    pEntry.dwSize = sizeof(pEntry);

    bool isParent = false;
    bool exitLoop = false;  // New flag to control the outer loop

    BOOL hRes = Process32FirstW(hSnapShot, &pEntry);
    while (hRes && !exitLoop) {  // Added check for exitLoop flag here
        if (wcscmp(pEntry.szExeFile, processName) == 0) {
            DWORD parentId = pEntry.th32ParentProcessID;

            //Log("%ls _______ processId : %lu\r\n", processName, pEntry.th32ProcessID);
            //Log("%ls _______ parentId : %lu\r\n", processName, parentId);

            // Reset snapshot to the beginning.
            Process32FirstW(hSnapShot, &pEntry);

            while (Process32NextW(hSnapShot, &pEntry)) {
                if (pEntry.th32ProcessID == parentId) {
                    if (wcscmp(pEntry.szExeFile, processName) != 0) {
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
DWORD isSelfChildProcessOfCurrent(const wchar_t* processName) {
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
        if (wcscmp(pEntry.szExeFile, processName) == 0) {
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


// A function to retrieve all child processes.
DWORD* GetAllChildProcesses(DWORD parentPid, int* count) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    DWORD staticArray[1024];
    int idx = 0;

    if (snapshot == INVALID_HANDLE_VALUE) {
        *count = 0;
        return NULL;
    }

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);

    if (!Process32First(snapshot, &entry)) {
        CloseHandle(snapshot);
        *count = 0;
        return NULL;
    }

    do {
        if (entry.th32ParentProcessID == parentPid) {
            staticArray[idx++] = entry.th32ProcessID;
        }
    } while (Process32Next(snapshot, &entry));

    CloseHandle(snapshot);

    *count = idx;
    DWORD* resultArray = (DWORD*)malloc(sizeof(DWORD) * idx);
    if (resultArray) {
        memcpy(resultArray, staticArray, sizeof(DWORD) * idx);
    }

    return resultArray;
}
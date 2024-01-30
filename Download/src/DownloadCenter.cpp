#include "framework.h"
#include "ServiceSource.h"
#include "DownloadCenter.h"
#include "BaseFileOpt.h"
#include "Log.h"
#include "DownloadThread.h"
#include "RichEditControls.h"

CRITICAL_SECTION progressCriticalSection;

// Note: When using CRITICAL_SECTION, you must not call CloseHandle on a thread created within its block; otherwise, 
// an error will occur. In other words, the thread handle must remain valid until DeleteCriticalSection is called, 
// and you cannot delete it.
DWORD WINAPI DispatchManagerDownloadThread(LPVOID lParam) {
    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_DOWNLOAD), FALSE);
    InitializeCriticalSection(&progressCriticalSection);
    SoftwareInfo* softwareItems[4];
    SoftwareGroupInfo* softwareGroupInfo = (SoftwareGroupInfo*)lParam;

    softwareItems[0] = &softwareGroupInfo->php;
    softwareItems[1] = &softwareGroupInfo->mysql;
    softwareItems[2] = &softwareGroupInfo->apache;
    softwareItems[3] = &softwareGroupInfo->nginx;

    for (int i = 0; i < 4; i++) {
        if (strlen(softwareItems[i]->link) > 0) {
            FileList* fileList = NULL;
            if (CheckDownloadFileExists(softwareItems[i]->version, &fileList)) {
                char tempVersionDirectory[1024];
                sprintf_s(tempVersionDirectory, sizeof(tempVersionDirectory), "%s/%s/%s",
                    DIRECTORY_SERVICE, softwareItems[i]->serviceType, softwareItems[i]->version);

                if (DirectoryExists(tempVersionDirectory)) {
                    continue;
                }

                char unzipTempMsg[512] = { '\0' };
                sprintf_s(unzipTempMsg, sizeof(unzipTempMsg), "Target file %s exists\r\n", softwareItems[i]->fileFullName);
                WarnOutput(unzipTempMsg);

                UnzipFile(softwareItems[i]);

                continue;
            }

            FreeCheckDownloadFileExists(fileList);

            // UnzipFile
            HANDLE managerThread = CreateThread(NULL, 0, DownloadManagerThread, softwareItems[i], 0, NULL);
            if (managerThread) {
                WaitForSingleObject(managerThread, INFINITE);
                CloseHandle(managerThread);
            }

        }
    }

    DeleteCriticalSection(&progressCriticalSection);

    EnableWindow(GetDlgItem(hWndMain, IDC_BUTTON_DOWNLOAD), TRUE);

    FreeSoftwareGroupInfo(softwareGroupInfo);
    return 0;
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

    int len = strlen(str);

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

void StartDownload(SoftwareGroupInfo softwareGroupInfo) {
    if (softwareGroupInfo.php.version != NULL && !CheckVersionSystemEnable(softwareGroupInfo.php.version)) {
        MessageBoxA(hWndMain, "Your system uses the x86 architecture; please download the compressed file with the suffix 'x86'.", NULL, 0);
        return;
    }

    if (softwareGroupInfo.mysql.version != NULL && !CheckVersionSystemEnable(softwareGroupInfo.mysql.version)) {
        MessageBoxA(hWndMain, "Your system uses the x86 architecture; please download the compressed file with the suffix 'x86'.", NULL, 0);
        return;
    }

    if (softwareGroupInfo.apache.version != NULL && !CheckVersionSystemEnable(softwareGroupInfo.apache.version)) {
        MessageBoxA(hWndMain, "Your system uses the x86 architecture; please download the compressed file with the suffix 'x86'.", NULL, 0);
        return;
    }

    // Determine whether the corresponding version can be downloaded.
    int major, minor;
    if (softwareGroupInfo.php.versionNumber != NULL) {
        if (sscanf_s(softwareGroupInfo.php.versionNumber, "php-%d.%d", &major, &minor) == 2) {
            if (major >= 8 && minor >= 2) {
                if (!IsWindows8OrGreater()) {
                    MessageBoxA(hWndMain, "When the PHP version is 8.2.0 or higher, the minimum operating system requirement is Windows 8.", NULL, 0);
                    return;
                }
            }
        }
        else {
            MessageBoxA(hWndMain, "php version error", NULL, 0);
            return;
        }
    }

    SoftwareGroupInfo* pSoftwareGroupInfo = DeepCopySoftwareGroupInfo(&softwareGroupInfo);
    HANDLE ThreadId = CreateThread(NULL, 0, DispatchManagerDownloadThread, pSoftwareGroupInfo, 0, NULL);

    FreeSoftwareInfo(&softwareGroupInfo.php);
    FreeSoftwareInfo(&softwareGroupInfo.mysql);
    FreeSoftwareInfo(&softwareGroupInfo.apache);
    FreeSoftwareInfo(&softwareGroupInfo.nginx);

    if (ThreadId) {
        CloseHandle(ThreadId);
    }
}

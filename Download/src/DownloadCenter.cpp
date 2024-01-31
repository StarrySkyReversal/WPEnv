#include "framework.h"
#include "ServiceSource.h"
#include "DownloadCenter.h"
#include "BaseFileOpt.h"
#include "Log.h"
#include "DownloadThread.h"
#include "RichEditControls.h"
#include "Common.h"

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

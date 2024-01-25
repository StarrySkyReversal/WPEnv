#include "framework.h"
#include "ServiceSource.h"
#include "DownloadCenter.h"
#include "BaseFileOpt.h"
#include "Log.h"
#include "DownloadThread.h"
#include "RichEditControls.h"

CRITICAL_SECTION progressCriticalSection;

void FreeSoftwareInfo(SoftwareInfo* info) {
    if (info->version != NULL && strcmp(info->version, "") != 0) {
        free((void*)info->serviceType);
        free((void*)info->version);
        free((void*)info->versionNumber);
        free((void*)info->link);
        free((void*)info->fileFullName);
    }
}

void FreeSoftwareGroupInfo(SoftwareGroupInfo* group) {
    if (!group) return;
    FreeSoftwareInfo(&group->php);
    FreeSoftwareInfo(&group->mysql);
    FreeSoftwareInfo(&group->apache);
    FreeSoftwareInfo(&group->nginx);

    delete group;
}

SoftwareInfo DeepCopySoftwareInfo(const SoftwareInfo* source, const char* serviceType) {
    SoftwareInfo dest;
    if (source->version != NULL) {
        dest.serviceType = _strdup(serviceType);
        dest.version = _strdup(source->version);
        dest.versionNumber = _strdup(source->versionNumber);
        dest.link = _strdup(source->link);
        dest.fileFullName = _strdup(source->fileFullName);
    }
    else {
        dest.serviceType = "";
        dest.version = "";
        dest.versionNumber = "";
        dest.link = "";
        dest.fileFullName = "";
    }

    return dest;
}

SoftwareGroupInfo* DeepCopySoftwareGroupInfo(const SoftwareGroupInfo* source) {
    SoftwareGroupInfo* dest = new SoftwareGroupInfo;

    dest->php = DeepCopySoftwareInfo(&source->php, "php");
    dest->mysql = DeepCopySoftwareInfo(&source->mysql, "mysql");
    dest->apache = DeepCopySoftwareInfo(&source->apache, "apache");
    dest->nginx = DeepCopySoftwareInfo(&source->nginx, "nginx");

    return dest;
}

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
                    //char serverFolderMsg[512];
                    //sprintf_s(serverFolderMsg, sizeof(serverFolderMsg), "The corresponding version folder already exists: %s/%s/%s\r\n",
                    //    DIRECTORY_SERVICE, softwareItems[i]->serviceType, softwareItems[i]->version);
                    //AppendEditInfo(serverFolderMsg);

                    continue;
                }

                char unzipTempMsg[512] = { '\0' };
                sprintf_s(unzipTempMsg, sizeof(unzipTempMsg), "INFO: Target file %s exists\r\n", softwareItems[i]->fileFullName);
                AppendEditInfo(unzipTempMsg);

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
    //extract_zip_file("./downloads/8.2.10-nts-Win32-vs16-x64.zip", "php", "8.2.10-nts-Win32-vs16-x64");
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

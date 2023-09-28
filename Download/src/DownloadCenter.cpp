#include "framework.h"
#include "ServiceSource.h"
#include "DownloadCenter.h"
#include "BaseFileOpt.h"
#include "Log.h"
#include "DownloadThread.h"
#include "RichEditControls.h"

CRITICAL_SECTION progressCriticalSection;

void FreeSoftwareInfo(SoftwareInfo* info) {
    if (info->version != NULL && wcscmp(info->version, L"") != 0) {
        free((void*)info->serviceType);
        free((void*)info->version);
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

SoftwareInfo DeepCopySoftwareInfo(const SoftwareInfo* source, const wchar_t* serviceType) {
    SoftwareInfo dest;
    if (source->version != NULL) {
        dest.serviceType = _wcsdup(serviceType);
        dest.version = _wcsdup(source->version);
        dest.link = _wcsdup(source->link);
        dest.fileFullName = _wcsdup(source->fileFullName);
    }
    else {
        dest.serviceType = L"";
        dest.version = L"";
        dest.link = L"";
        dest.fileFullName = L"";
    }

    return dest;
}

SoftwareGroupInfo* DeepCopySoftwareGroupInfo(const SoftwareGroupInfo* source) {
    SoftwareGroupInfo* dest = new SoftwareGroupInfo;

    dest->php = DeepCopySoftwareInfo(&source->php, L"php");
    dest->mysql = DeepCopySoftwareInfo(&source->mysql, L"mysql");
    dest->apache = DeepCopySoftwareInfo(&source->apache, L"apache");
    dest->nginx = DeepCopySoftwareInfo(&source->nginx, L"nginx");

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
        if (wcslen(softwareItems[i]->link) > 0) {
            FileList* fileList = NULL;
            if (CheckDownloadFileExists(softwareItems[i]->version, &fileList)) {
                wchar_t tempVersionDirectory[1024];
                swprintf_s(tempVersionDirectory, sizeof(tempVersionDirectory) / sizeof(wchar_t), L"%ls/%ls/%ls",
                    DIRECTORY_SERVICE, softwareItems[i]->serviceType, softwareItems[i]->version);

                if (DirectoryExists(tempVersionDirectory)) {
                    wchar_t serverFolderMsg[512];
                    swprintf_s(serverFolderMsg, _countof(serverFolderMsg), L"The corresponding version folder already exists: %ls/%ls/%ls\r\n",
                        DIRECTORY_SERVICE, softwareItems[i]->serviceType, softwareItems[i]->version);
                    AppendEditInfo(serverFolderMsg);

                    continue;
                }

                wchar_t unzipTempMsg[512] = { '\0' };
                swprintf_s(unzipTempMsg, sizeof(unzipTempMsg) / sizeof(wchar_t), L"INFO: Target file %ls exists\r\n", softwareItems[i]->fileFullName);
                AppendEditInfo(unzipTempMsg);

                UnzipFile(softwareItems[i]);

                //Log(L"Download failed, file version %ls exists.\r\n", softwareItems[i]->version);
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

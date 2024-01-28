#include "framework.h"
#include <stdio.h>
#include "ListViewControls.h"
#include "WindowLayout.h"
#include "Log.h"
#include "FileModify.h"
#include "FileFindOpt.h"
#include "BaseFileOpt.h"
#include "Common.h"
#include "FindPattern.h"
#include "SyncServiceConfig.h"
#include "ServiceUse.h"
#include "ServiceSource.h"

ServiceUseConfig serviceUseConfigs[256] = { 0 };

int ReadServiceUseConfig(const char* filePath, ServiceUseConfig* configs) {
    FILE* file;
    fopen_s(&file, filePath, "r");
    if (file == NULL) {
        return 0;
    }

    char line[256];
    int index = 0;

    while (fgets(line, sizeof(line), file) && index < 256) {
        char* newline = strchr(line, L'\n');
        if (newline) *newline = '\0';

        char* token = NULL;
        char* next_token = NULL;

        // Decompose configuration
        token = strtok_s(line, "|", &next_token);
        if (token) strcpy_s(configs[index].php, sizeof(configs[index].php), token);

        token = strtok_s(NULL, "|", &next_token);
        if (token) strcpy_s(configs[index].mysql, sizeof(configs[index].mysql), token);

        token = strtok_s(NULL, "|", &next_token);
        if (token) strcpy_s(configs[index].webService, sizeof(configs[index].webService), token);

        index++;
    }

    fclose(file);
    return index;
}

int AddConfigToServiceUse(const char* filePath, const char* config) {
    FILE* file;
    fopen_s(&file, filePath, "a");
    if (file == NULL) {
        return 0;
    }

    fprintf(file, "%s\n", config);
    fclose(file);
    return 1;
}

int RemoveServiceUseItem(const char* filePath, int itemIndex) {
    FILE* file;
    if (fopen_s(&file, filePath, "r") != 0 || file == NULL) {
        return 0;
    }

    char line[256];
    char tempPath[256];

    strcpy_s(tempPath, sizeof(tempPath), filePath);
    strcat_s(tempPath, sizeof(tempPath), ".tmp_webservice");  // Create a temporary filename

    FILE* tempFile;
    if (fopen_s(&tempFile, tempPath, "w") != 0 || tempFile == NULL) {
        fclose(file);
        return 0;
    }

    int currentIndex = 0;
    while (fgets(line, sizeof(line), file)) {
        if (currentIndex != itemIndex) {
            fputs(line, tempFile);
        }
        currentIndex++;
    }

    fclose(file);
    fclose(tempFile);

    // Replace original file
    if (DeleteFileA(filePath) == 0) {
        DeleteFileA(tempPath);
        return 0;
    }

    if (rename(tempPath, filePath) != 0) {
        return 0;
    }

    return 1;
}

void LoadServiceUseDataToListView() {
    serviceUseConfigs->itemCount = ReadServiceUseConfig(FILE_CONFIG_SERVICE_USE, serviceUseConfigs);

    int lineNumber = 0;
    for (int i = 0; i < serviceUseConfigs->itemCount; i++) {
        int insertIndex = GetListViewItemInsertIndex(hListConfig);

        char firstInfo[32];
        sprintf_s(firstInfo, sizeof(firstInfo), "%i", ++lineNumber);

        AddListViewItem(hListConfig, insertIndex, 0, firstInfo);
        AddListViewItem(hListConfig, insertIndex, 1, serviceUseConfigs[i].php);
        AddListViewItem(hListConfig, insertIndex, 2, serviceUseConfigs[i].mysql);
        AddListViewItem(hListConfig, insertIndex, 3, serviceUseConfigs[i].webService);
    }
}

bool AddNewConfig(SoftwareGroupInfo softwareGroupInfo) {
    SoftwareGroupInfo pSoftwareGroupInfo;

    memset(&pSoftwareGroupInfo, 0, sizeof(SoftwareGroupInfo));
    GetServiceVersionInfo(&pSoftwareGroupInfo);

    if (!(
        softwareGroupInfo.php.version != NULL && softwareGroupInfo.mysql.version != NULL && (softwareGroupInfo.apache.version != NULL || softwareGroupInfo.nginx.version != NULL))
        ) {
        LogAndMsgBox(L"Config is not found.");
        return false;
    }

    if (softwareGroupInfo.apache.version != NULL && softwareGroupInfo.nginx.version != NULL) {
        LogAndMsgBox(L"Apache and Nginx can only choose one.");
        return false;
    }

    const char* service;
    if (softwareGroupInfo.apache.version) {
        service = softwareGroupInfo.apache.version;
    }
    else {
        service = softwareGroupInfo.nginx.version;
    }

    if (strstr(softwareGroupInfo.php.version, "nts") != NULL && strcmp(service, "apache") == 0) {
        LogAndMsgBox("php apache is not found ts version");

        return false;
    }

    char wCombinedConfig[1024];
    sprintf_s(wCombinedConfig, sizeof(wCombinedConfig), "%s|%s|%s",
        softwareGroupInfo.php.version,
        softwareGroupInfo.mysql.version,
        service
    );

    int configCount = ReadServiceUseConfig(FILE_CONFIG_SERVICE_USE, serviceUseConfigs);
    if (configCount > 255) {
        LogAndMsgBox(L"Too many configurations");
        return false;
    }

    for (int i = 0; i < configCount; i++) {
        char existsConfigRecord[1024];
        sprintf_s(existsConfigRecord, sizeof(existsConfigRecord),
            "%s|%s|%s", serviceUseConfigs[i].php, serviceUseConfigs[i].mysql, serviceUseConfigs[i].webService);

        if (strcmp(existsConfigRecord, wCombinedConfig) == 0) {
            LogAndMsgBox(L"Configuration already exists.");
            return false;
        }
    }

    AddConfigToServiceUse(FILE_CONFIG_SERVICE_USE, wCombinedConfig);

    int itemIndex = GetListViewItemInsertIndex(hListConfig);
    int nCount = GetListViewCount(hListConfig);

    // We don't need to add 1 here. If we do, it actually increases by 1. 
    // It's an issue with the value returned by the control's property. 

    // int nNo = nCount + 1;
    char firstInfo[32] = { '\0' };
    sprintf_s(firstInfo, sizeof(firstInfo), "%i", nCount);

    AddListViewItem(hListConfig, itemIndex, 0, firstInfo);
    AddListViewItem(hListConfig, itemIndex, 1, softwareGroupInfo.php.version);
    AddListViewItem(hListConfig, itemIndex, 2, softwareGroupInfo.mysql.version);
    AddListViewItem(hListConfig, itemIndex, 3, service);

    if (webDaemonServiceInstance.bRun == false) {
        SetListViewFocus();
    }

    return true;
}

void RemoveListViewSelectedItem() {
    int selectedIndex = GetListViewSelectedIndex(hListConfig);
    int serviceUseFileIndex = GetListViewCount(hListConfig) - selectedIndex -1;
    
    if (selectedIndex >= 0) {
        if (RemoveServiceUseItem(FILE_CONFIG_SERVICE_USE, serviceUseFileIndex)) {
            if (!DeleteListViewItem(hListConfig, selectedIndex)) {
                LogAndMsgBox("Delete fail\r\n");
            }
            else {
                UpdateListViewSortNumber();
                // Update list sort Number
                //ListView_Set
            }
        }
    }
}

bool GetServiceUseItem(ServiceUseConfig* config) {
    int selectedIndex = GetListViewSelectedIndex(hListConfig);
    if (selectedIndex == -1) {
        return false;
    }

    char phpBuffer[512] = { '\0' };
    char mysqlBuffer[512] = { '\0' };
    char serviceBuffer[512] = { '\0' };

    GetListViewSelectedText(hListConfig, selectedIndex, 1, phpBuffer, sizeof(phpBuffer));
    GetListViewSelectedText(hListConfig, selectedIndex, 2, mysqlBuffer, sizeof(mysqlBuffer));
    GetListViewSelectedText(hListConfig, selectedIndex, 3, serviceBuffer, sizeof(serviceBuffer));

    char textBuffer[512] = { '\0' };
    // Here, to maintain compatibility with the original code, we still use the '|' as a delimiter.
    sprintf_s(textBuffer, sizeof(textBuffer), "%s|%s|%s", phpBuffer, mysqlBuffer, serviceBuffer);

    char* token = NULL;
    char* next_token = NULL;

    config->itemCount = 1;

    token = strtok_s(textBuffer, "|", &next_token);
    if (token) strcpy_s(config->php, sizeof(config->php), token);

    token = strtok_s(NULL, "|", &next_token);
    if (token) strcpy_s(config->mysql, sizeof(config->mysql), token);

    token = strtok_s(NULL, "|", &next_token);
    if (token) strcpy_s(config->webService, sizeof(config->webService), token);

    return true;
}

void getServiceVersionDirectory(const char* serviceType, const char* version, char* buffer, int bufferSize) {
    sprintf_s(buffer, bufferSize, "%s/%s/%s",
        DIRECTORY_SERVICE, serviceType, version);
}

void getApacheVersionAbsBaseDir(const char* version, char* buffer, int bufferSize) {
    char serviceVersionDir[256];
    getServiceVersionDirectory("apache", version, serviceVersionDir, sizeof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, "httpd.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The apache directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    char currentDirBuffer[256];
    GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, sizeof(currentDirBuffer), true);

    char* programBaseDir = get_current_program_directory_with_forward_slash();
    sprintf_s(buffer, bufferSize, "%s/%s", programBaseDir, currentDirBuffer);

    freePathList(pathsList);
}

void getPhpAbsBaseDir(const char* version, char* buffer, int bufferSize) {
    char serviceVersionDir[256];
    getServiceVersionDirectory("php", version, serviceVersionDir, sizeof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, "php-cgi.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The php directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    if (pathsList->count == 1) {
        char currentDirBuffer[256];
        GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, sizeof(currentDirBuffer), false);

        char* programBaseDir = get_current_program_directory_with_forward_slash();
        sprintf_s(buffer, bufferSize, "%s/%s", programBaseDir, currentDirBuffer);
    }

    freePathList(pathsList);
}

void getNginxAbsBaseDir(const char* version, char* buffer, int bufferSize) {
    char serviceVersionDir[256];
    getServiceVersionDirectory("nginx", version, serviceVersionDir, sizeof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, "nginx.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The nginx directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    char currentDirBuffer[256];
    GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, sizeof(currentDirBuffer), false);

    char* programBaseDir = get_current_program_directory_with_forward_slash();
    sprintf_s(buffer, bufferSize, "%s/%s", programBaseDir, currentDirBuffer);

    freePathList(pathsList);
}

void getMysqlAbsBaseDir(const char* version, char* buffer, int bufferSize) {
    char serviceVersionDir[256];
    getServiceVersionDirectory("mysql", version, serviceVersionDir, sizeof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, "mysqld.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The mysql directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    char currentDirBuffer[256];
    GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, sizeof(currentDirBuffer), true);

    char* programBaseDir = get_current_program_directory_with_forward_slash();
    sprintf_s(buffer, bufferSize, "%s/%s", programBaseDir, currentDirBuffer);

    freePathList(pathsList);
}

void getPhpDevIniOriginalFilePath(SoftwareInfo softwareInfo, char* buffer, int bufferSize) {
    sprintf_s(buffer, bufferSize, "%s/%s/%s/php.ini-development",
        DIRECTORY_SERVICE, softwareInfo.serviceType, softwareInfo.version);
}

void getApacheHttpdConfFilePath(const char* version, char* buffer, int bufferSize) {
    char apacheAbsBaseDir[256] = { L'\0' };
    getApacheVersionAbsBaseDir(version, apacheAbsBaseDir, sizeof(apacheAbsBaseDir));

    if (apacheAbsBaseDir[0] != L'\0') {
        sprintf_s(buffer, bufferSize, "%s/conf/httpd.conf", apacheAbsBaseDir);
    }
}

void getNginxConfFilePath(const char* version, char* buffer, int bufferSize) {
    char nginxAbsBaseDir[256] = { '\0' };
    getNginxAbsBaseDir(version, nginxAbsBaseDir, sizeof(nginxAbsBaseDir));

    if (nginxAbsBaseDir[0] != L'\0') {
        sprintf_s(buffer, bufferSize, "%s/conf/nginx.conf", nginxAbsBaseDir);
    }
}

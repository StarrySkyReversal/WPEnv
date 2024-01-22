#include "framework.h"
#include "ServiceUse.h"
#include "ServiceSource.h"
#include <stdio.h>
#include "ListViewControls.h"
#include "WindowLayout.h"
#include "Log.h"
#include "FileModify.h"
#include "FileFindOpt.h"
#include "BaseFileOpt.h"
#include "Common.h"
#include "FindPattern.h"

ServiceUseConfig serviceUseConfigs[MAX_CONFIGS] = { 0 };

int ReadServiceUseConfig(const wchar_t* filePath, ServiceUseConfig* configs) {
    FILE* file;
    _wfopen_s(&file, filePath, L"r");
    if (file == NULL) {
        return 0;
    }

    wchar_t line[MAX_CONFIG_LEN];
    int index = 0;

    while (fgetws(line, _countof(line), file) && index < MAX_CONFIGS) {
        wchar_t* newline = wcschr(line, L'\n');
        if (newline) *newline = '\0';

        wchar_t* token = NULL;
        wchar_t* next_token = NULL;

        // Decompose configuration
        token = wcstok_s(line, L"|", &next_token);
        if (token) wcscpy_s(configs[index].php, _countof(configs[index].php), token);

        token = wcstok_s(NULL, L"|", &next_token);
        if (token) wcscpy_s(configs[index].mysql, _countof(configs[index].mysql), token);

        token = wcstok_s(NULL, L"|", &next_token);
        if (token) wcscpy_s(configs[index].webService, _countof(configs[index].webService), token);

        index++;
    }

    fclose(file);
    return index;
}

int AddConfigToServiceUse(const wchar_t* filePath, const wchar_t* config) {
    FILE* file;
    _wfopen_s(&file, filePath, L"a");
    if (file == NULL) {
        return 0;
    }

    fwprintf(file, L"%ls\n", config);
    fclose(file);
    return 1;
}

int RemoveServiceUseItem(const wchar_t* filePath, int itemIndex) {
    FILE* file;
    if (_wfopen_s(&file, filePath, L"r") != 0 || file == NULL) {
        return 0;
    }

    wchar_t line[MAX_CONFIG_LEN];
    wchar_t tempPath[MAX_PATH];

    wcscpy_s(tempPath, _countof(tempPath), filePath);
    wcscat_s(tempPath, _countof(tempPath), L".tmp_webservice");  // Create a temporary filename

    FILE* tempFile;
    if (_wfopen_s(&tempFile, tempPath, L"w") != 0 || tempFile == NULL) {
        fclose(file);
        return 0;
    }

    int currentIndex = 0;
    while (fgetws(line, _countof(line), file)) {
        if (currentIndex != itemIndex) {
            fputws(line, tempFile);
        }
        currentIndex++;
    }

    fclose(file);
    fclose(tempFile);

    // Replace original file
    if (DeleteFile(filePath) == 0) {
        DeleteFile(tempPath);
        return 0;
    }

    if (_wrename(tempPath, filePath) != 0) {
        return 0;
    }

    return 1;
}

void LoadServiceUseDataToListView() {
    serviceUseConfigs->itemCount = ReadServiceUseConfig(FILE_CONFIG_SERVICE_USE, serviceUseConfigs);

    int lineNumber = 0;
    for (int i = 0; i < serviceUseConfigs->itemCount; i++) {
        int insertIndex = GetListViewItemInsertIndex(hListConfig);

        wchar_t firstInfo[32];
        swprintf_s(firstInfo, _countof(firstInfo), L"%i", ++lineNumber);

        AddListViewItem(hListConfig, insertIndex, 0, firstInfo);
        AddListViewItem(hListConfig, insertIndex, 1, (LPWSTR)serviceUseConfigs[i].php);
        AddListViewItem(hListConfig, insertIndex, 2, (LPWSTR)serviceUseConfigs[i].mysql);
        AddListViewItem(hListConfig, insertIndex, 3, (LPWSTR)serviceUseConfigs[i].webService);
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

    const wchar_t* service;
    if (softwareGroupInfo.apache.version) {
        service = softwareGroupInfo.apache.version;
    }
    else {
        service = softwareGroupInfo.nginx.version;
    }

    if (wcsstr(softwareGroupInfo.php.version, L"nts") != NULL && wcscmp(service, L"apache") == 0) {
        LogAndMsgBox("php apache is not found ts version");

        return false;
    }

    wchar_t wCombinedConfig[1024];
    swprintf_s(wCombinedConfig, sizeof(wCombinedConfig) / sizeof(wchar_t), L"%ls|%ls|%ls",
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
        wchar_t existsConfigRecord[1024];
        swprintf_s(existsConfigRecord, _countof(existsConfigRecord),
            L"%ls|%ls|%ls", serviceUseConfigs[i].php, serviceUseConfigs[i].mysql, serviceUseConfigs[i].webService);

        if (wcscmp(existsConfigRecord, wCombinedConfig) == 0) {
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
    wchar_t firstInfo[32] = { '\0' };
    swprintf_s(firstInfo, _countof(firstInfo), L"%i", nCount);

    AddListViewItem(hListConfig, itemIndex, 0, firstInfo);
    AddListViewItem(hListConfig, itemIndex, 1, (LPWSTR)softwareGroupInfo.php.version);
    AddListViewItem(hListConfig, itemIndex, 2, (LPWSTR)softwareGroupInfo.mysql.version);
    AddListViewItem(hListConfig, itemIndex, 3, (LPWSTR)service);

    return true;
}

void RemoveListViewSelectedItem() {
    int selectedIndex = GetListViewSelectedIndex(hListConfig);

    if (RemoveServiceUseItem(FILE_CONFIG_SERVICE_USE, selectedIndex)) {
        if (!DeleteListViewItem(hListConfig, selectedIndex)) {
            LogAndMsgBox("Delete fail\r\n");
        }
    }
}

bool GetServiceUseItem(ServiceUseConfig* config) {
    int selectedIndex = GetListViewSelectedIndex(hListConfig);
    if (selectedIndex == -1) {
        return false;
    }

    wchar_t phpBuffer[512] = { '\0' };
    wchar_t mysqlBuffer[512] = { '\0' };
    wchar_t serviceBuffer[512] = { '\0' };

    GetListViewSelectedText(hListConfig, selectedIndex, 1, phpBuffer, _countof(phpBuffer));
    GetListViewSelectedText(hListConfig, selectedIndex, 2, mysqlBuffer, _countof(mysqlBuffer));
    GetListViewSelectedText(hListConfig, selectedIndex, 3, serviceBuffer, _countof(serviceBuffer));

    wchar_t textBuffer[512] = { '\0' };
    // Here, to maintain compatibility with the original code, we still use the '|' as a delimiter.
    swprintf_s(textBuffer, _countof(textBuffer), L"%ls|%ls|%ls", phpBuffer, mysqlBuffer, serviceBuffer);

    wchar_t* token = NULL;
    wchar_t* next_token = NULL;

    config->itemCount = 1;

    token = wcstok_s(textBuffer, L"|", &next_token);
    if (token) wcscpy_s(config->php, _countof(config->php), token);

    token = wcstok_s(NULL, L"|", &next_token);
    if (token) wcscpy_s(config->mysql, _countof(config->mysql), token);

    token = wcstok_s(NULL, L"|", &next_token);
    if (token) wcscpy_s(config->webService, _countof(config->webService), token);

    return true;
}

void getServiceVersionDirectory(const wchar_t* serviceType, const wchar_t* version, wchar_t* buffer, int bufferSize) {
    swprintf_s(buffer, bufferSize, L"%ls/%ls/%ls",
        DIRECTORY_SERVICE, serviceType, version);
}

void getApacheVersionAbsBaseDir(const wchar_t* version, wchar_t* buffer, int bufferSize) {
    wchar_t serviceVersionDir[256];
    getServiceVersionDirectory(L"apache", version, serviceVersionDir, _countof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, L"httpd.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The apache directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    wchar_t currentDirBuffer[256];
    GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, _countof(currentDirBuffer), true);

    wchar_t* programBaseDir = get_current_program_directory_with_forward_slash();
    swprintf_s(buffer, bufferSize, L"%ls/%ls", programBaseDir, currentDirBuffer);

    freePathList(pathsList);
}

void getPhpAbsBaseDir(const wchar_t* version, wchar_t* buffer, int bufferSize) {
    wchar_t serviceVersionDir[256];
    getServiceVersionDirectory(L"php", version, serviceVersionDir, _countof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, L"php-cgi.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The php directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    if (pathsList->count == 1) {
        wchar_t currentDirBuffer[256];
        GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, _countof(currentDirBuffer), false);

        wchar_t* programBaseDir = get_current_program_directory_with_forward_slash();
        swprintf_s(buffer, bufferSize, L"%ls/%ls", programBaseDir, currentDirBuffer);
    }

    freePathList(pathsList);
}

void getNginxAbsBaseDir(const wchar_t* version, wchar_t* buffer, int bufferSize) {
    wchar_t serviceVersionDir[256];
    getServiceVersionDirectory(L"nginx", version, serviceVersionDir, _countof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, L"nginx.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The nginx directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    wchar_t currentDirBuffer[256];
    GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, _countof(currentDirBuffer), false);

    wchar_t* programBaseDir = get_current_program_directory_with_forward_slash();
    swprintf_s(buffer, bufferSize, L"%ls/%ls", programBaseDir, currentDirBuffer);

    freePathList(pathsList);
}

void getMysqlAbsBaseDir(const wchar_t* version, wchar_t* buffer, int bufferSize) {
    wchar_t serviceVersionDir[256];
    getServiceVersionDirectory(L"mysql", version, serviceVersionDir, _countof(serviceVersionDir));

    PathList* pathsList = initPathList();
    findFilesInDirectory(serviceVersionDir, L"mysqld.exe", pathsList);

    if (pathsList->count != 1) {
        LogAndMsgBox("The mysql directory does not exist.\r\n");

        freePathList(pathsList);
        return;
    }

    wchar_t currentDirBuffer[256];
    GetDirectoryFromPath(pathsList->paths[0], currentDirBuffer, _countof(currentDirBuffer), true);

    wchar_t* programBaseDir = get_current_program_directory_with_forward_slash();
    swprintf_s(buffer, bufferSize, L"%ls/%ls", programBaseDir, currentDirBuffer);

    freePathList(pathsList);
}

void getPhpDevIniOriginalFilePath(SoftwareInfo softwareInfo, wchar_t* buffer, int bufferSize) {
    swprintf_s(buffer, bufferSize, L"%ls/%ls/%ls/php.ini-development",
        DIRECTORY_SERVICE, softwareInfo.serviceType, softwareInfo.version);
}

void getApacheHttpdConfFilePath(const wchar_t* version, wchar_t* buffer, int bufferSize) {
    wchar_t apacheAbsBaseDir[256] = { L'\0' };
    getApacheVersionAbsBaseDir(version, apacheAbsBaseDir, _countof(apacheAbsBaseDir));

    if (apacheAbsBaseDir[0] != L'\0') {
        swprintf_s(buffer, bufferSize, L"%ls/conf/httpd.conf", apacheAbsBaseDir);
    }
}

void getNginxConfFilePath(const wchar_t* version, wchar_t* buffer, int bufferSize) {
    wchar_t nginxAbsBaseDir[256] = { L'\0' };
    getNginxAbsBaseDir(version, nginxAbsBaseDir, _countof(nginxAbsBaseDir));

    if (nginxAbsBaseDir[0] != L'\0') {
        swprintf_s(buffer, bufferSize, L"%ls/conf/nginx.conf", nginxAbsBaseDir);
    }
}


void InitializeNginxConfigFile(SoftwareInfo softwareInfo) {
    wchar_t* wProgramDirectory = get_current_program_directory_with_forward_slash();

    char programDirectory[256];
    WToM(wProgramDirectory, programDirectory, sizeof(programDirectory));

    char wwwDir[256];
    sprintf_s(wwwDir, sizeof(wwwDir), "root %s/www/default;", programDirectory);

    wchar_t wNginxConFilePath[256];
    getNginxConfFilePath(softwareInfo.version, wNginxConFilePath, _countof(wNginxConFilePath));

    char nginxConFilePath[256];
    WToM(wNginxConFilePath, nginxConFilePath, sizeof(nginxConFilePath));
    if (modify_conf_utf8AndAscii(nginxConFilePath, "root   html;", wwwDir, true) == 0) {
    }

    if (modify_conf_utf8AndAscii(nginxConFilePath, "index  index.html index.htm;", "index  index.php index.html index.htm;", true) == 0) {
    }

    remove_comment(nginxConFilePath, "#location ~ \\.php$ {", "#}");
}

void InitializeApacheConfigFile(SoftwareInfo softwareInfo) {
    wchar_t targetServiceDirectory[512] = { '\0' };
    swprintf_s(targetServiceDirectory, _countof(targetServiceDirectory), L"%ls/%ls/%ls",
        DIRECTORY_SERVICE, softwareInfo.serviceType, softwareInfo.version);

    PathList* pathsList = initPathList();
    findFilesInDirectory(targetServiceDirectory, L"httpd.exe", pathsList);
    if (pathsList->count == 0) {
        freePathList(pathsList);
        return;
    }

    wchar_t* wProgramDirectory = get_current_program_directory_with_forward_slash();

    char programDirectory[256];
    WToM(wProgramDirectory, programDirectory, sizeof(programDirectory));

    wchar_t wParentDirectory[256];
    GetDirectoryFromPath(pathsList->paths[0], wParentDirectory, sizeof(wParentDirectory) / sizeof(wchar_t), true);

    char parentDirectory[256];
    WToM(wParentDirectory, parentDirectory, sizeof(parentDirectory));

    char configFullPath[256];
    sprintf_s(configFullPath, sizeof(configFullPath), "%s/conf/httpd.conf", parentDirectory);

    char mainDirectory[512];
    sprintf_s(mainDirectory, sizeof(mainDirectory), "%s/%s", programDirectory, parentDirectory);

    char newDirectory[256];
    sprintf_s(newDirectory, sizeof(newDirectory), "Define SRVROOT \"%s\"", mainDirectory);

    if (modify_conf_line_utf8AndAscii(configFullPath, "Define SRVROOT", newDirectory) == 0) {
    }

    char wwwDir[256];
    sprintf_s(wwwDir, sizeof(wwwDir), "%s/www", programDirectory);

    char newDocumentRootDir[256];
    sprintf_s(newDocumentRootDir, sizeof(newDocumentRootDir), "DocumentRoot \"%s\"", wwwDir);
    if (modify_conf_line_utf8AndAscii(configFullPath, "DocumentRoot \"${SRVROOT}/htdocs\"", newDocumentRootDir) == 0) {
    }

    char newServerName[] = "ServerName localhost:80";
    if (modify_conf_line_utf8AndAscii(configFullPath, "#ServerName www.example.com:80", newServerName) == 0) {
    }

    char newDirectoryIndex[] = "DirectoryIndex index.php index.html";
    if (modify_conf_utf8AndAscii(configFullPath, "DirectoryIndex index.html", newDirectoryIndex) == 0) {
    }

    char newDirectoryDir[256];
    sprintf_s(newDirectoryDir, sizeof(newDirectoryDir), "<Directory \"%s\">", wwwDir);
    if (modify_conf_line_utf8AndAscii(configFullPath, "<Directory \"${SRVROOT}/htdocs\">", newDirectoryDir) == 0) {
    }

    if (modify_conf_line_utf8AndAscii(configFullPath, "#Include conf/extra/httpd-vhosts.conf", "Include conf/extra/httpd-vhosts.conf") == 0) {

        char vhostsFullPath[1024];
        sprintf_s(vhostsFullPath, sizeof(vhostsFullPath), "%s/conf/extra/httpd-vhosts.conf", parentDirectory);

        if (comment_block(vhostsFullPath, "<VirtualHost *:80>", "</VirtualHost>") == 0) {
            const char virtualHostBegin[] = "\r\n<VirtualHost *:80>\r\n";
            char virtualHostDocumentRoot[512];
            const char virtualHostServerName[] = "    ServerName localhost\r\n";
            char virtualHostErrorLog[512] = "    ErrorLog \"logs/localhost-error.log\"\r\n";
            char virtualHostCustomLog[512] = "    CustomLog \"logs/localhost-access.log\" common\r\n";
            const char virtualHostEnd[] = "</VirtualHost>\r\n";

            char virtualHostConf[4096];

            sprintf_s(virtualHostDocumentRoot, sizeof(virtualHostDocumentRoot), "    DocumentRoot \"%s/default\"\r\n", wwwDir);
            sprintf_s(virtualHostConf, sizeof(virtualHostConf), "%s%s%s%s%s%s",
                virtualHostBegin,
                virtualHostDocumentRoot,
                virtualHostServerName,
                virtualHostErrorLog,
                virtualHostCustomLog,
                virtualHostEnd);

            FILE* file;
            fopen_s(&file, vhostsFullPath, "a");
            if (file) {
                fputs(virtualHostConf, file);
                fclose(file);

                Log("VhostsFullPath edit OK\r\n");
            }

        }
    }

    freePathList(pathsList);
}

int matchPattern(const wchar_t* filename) {
    if (wcsstr(filename, L"php") == filename) {
        if (wcsstr(filename, L"apache") && wcsstr(filename, L".dll") == filename + wcslen(filename) - 4) {
            return 1;  // Match found
        }
    }
    return 0;  // No match
}

void findMatchingFiles(const wchar_t* directory, wchar_t* buffer, int bufferSize) {
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wchar_t dirSpec[MAX_PATH];
    _snwprintf_s(dirSpec, sizeof(dirSpec) / sizeof(wchar_t), L"%s\\*", directory);

    hFind = FindFirstFile(dirSpec, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        wprintf(L"Invalid file handle. Error is %u.\n", GetLastError());

        return;
    }
    else {
        do {
            if (matchPattern(findFileData.cFileName)) {
                wcscpy_s(buffer, bufferSize, findFileData.cFileName);

                FindClose(hFind);
                return;
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}

void syncPHPConfigFile(SoftwareInfo softwareInfo) {
    wchar_t wPhpIniFilePath[256];
    swprintf_s(wPhpIniFilePath, _countof(wPhpIniFilePath), L"%ls/%ls/%ls/php.ini-development",
        DIRECTORY_SERVICE, softwareInfo.serviceType, softwareInfo.version);

    wchar_t wPhpIniFilePathDest[256];
    swprintf_s(wPhpIniFilePathDest, _countof(wPhpIniFilePathDest), L"%ls/%ls/%ls/php.ini",
        DIRECTORY_SERVICE, softwareInfo.serviceType, softwareInfo.version);

    char phpIniFilePathSource[256];
    WToM(wPhpIniFilePath, phpIniFilePathSource, sizeof(phpIniFilePathSource));

    char phpIniFilePathDest[256];
    WToM(wPhpIniFilePathDest, phpIniFilePathDest, sizeof(phpIniFilePathDest));

    // copy php.ini-development to php.ini
    if (copyFile(phpIniFilePathSource, phpIniFilePathDest) == 0) {
        char phpExtensionDir[] = "extension_dir = ext";
        if (modify_conf_line_utf8AndAscii(phpIniFilePathDest, ";extension_dir = \"ext\"", phpExtensionDir) == 0) {
        }

        char phpIniExtCurl[] = "extension=curl";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=curl", phpIniExtCurl) == 0) {
        }

        char phpIniExtFileInfo[] = "extension=fileinfo";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=fileinfo", phpIniExtFileInfo) == 0) {
        }

        char phpIniExtGd[] = "extension=gd";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=gd", phpIniExtGd) == 0) {
        }

        char phpIniExtMbString[] = "extension=mbstring";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=mbstring", phpIniExtMbString) == 0) {
        }

        char phpIniExtOpenssl[] = "extension=openssl";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=openssl", phpIniExtOpenssl) == 0) {
        }

        char phpIniExtMysqli[] = "extension=mysqli";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=mysqli", phpIniExtMysqli) == 0) {
        }

        char phpIniExtPdoMysql[] = "extension=pdo_mysql";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=pdo_mysql", phpIniExtPdoMysql) == 0) {
        }

        char phpIniExtZip[] = "extension=zip";
        if (modify_conf_utf8AndAscii(phpIniFilePathDest, ";extension=zip", phpIniExtZip) == 0) {
        }
    }
}

void syncConfigFilePHPAndApache(const wchar_t* phpVersion, const wchar_t* apacheVersion) {
    wchar_t phpDir[256] = { L'0' };
    swprintf_s(phpDir, _countof(phpDir), L"%ls/php/%ls", DIRECTORY_SERVICE, phpVersion);

    wchar_t phpApacheDllFileName[256] = { L'0' };
    findMatchingFiles(phpDir, phpApacheDllFileName, _countof(phpApacheDllFileName));
    
    wchar_t phpAbsBaseDir[256];
    getPhpAbsBaseDir(phpVersion, phpAbsBaseDir, _countof(phpAbsBaseDir));

    wchar_t wApacheHttpdConFilePath[256];
    getApacheHttpdConfFilePath(apacheVersion, wApacheHttpdConFilePath, _countof(wApacheHttpdConFilePath));

    int versionNumber;
    if (swscanf_s(phpApacheDllFileName + 3, L"%d", &versionNumber) != 1) {
        Log("phpApacheDllFileName ERROR\r\n");
        return;
    }

    char apacheHttpdConFilePath[256];
    WToM(wApacheHttpdConFilePath, apacheHttpdConFilePath, sizeof(apacheHttpdConFilePath));

    wchar_t wPhpPathAndDllPath[256];
    char phpPathAndDllPath[256];

    char moduleContent[256] = { '\0' };
    search_file_for_pattern(apacheHttpdConFilePath, "LoadModule php?_module", moduleContent, sizeof(moduleContent));
    wchar_t wModuleContent[256] = { L'\0' };
    MToW(moduleContent, wModuleContent, _countof(wModuleContent));

    swprintf_s(wPhpPathAndDllPath, _countof(wPhpPathAndDllPath), L"LoadModule %ls \"%ls/%ls\"\r\n", wModuleContent, phpAbsBaseDir, phpApacheDllFileName);
    WToM(wPhpPathAndDllPath, phpPathAndDllPath, sizeof(phpPathAndDllPath));

    if (versionNumber == 5) {
        if (moduleContent[0] == '\0') {
            strcpy_s(moduleContent, sizeof(moduleContent), "LoadModule php5_module");
        }

        swprintf_s(wPhpPathAndDllPath, _countof(wPhpPathAndDllPath), L"LoadModule php5_module \"%ls/%ls\"\r\n", phpAbsBaseDir, phpApacheDllFileName);

        WToM(wPhpPathAndDllPath, phpPathAndDllPath, sizeof(phpPathAndDllPath));
        modify_conf_line_InsertOrUpdate_utf8AndAscii(apacheHttpdConFilePath, moduleContent, phpPathAndDllPath);

    }
    else if (versionNumber == 7) {
        if (moduleContent[0] == '\0') {
            strcpy_s(moduleContent, sizeof(moduleContent), "LoadModule php7_module");
        }

        swprintf_s(wPhpPathAndDllPath, _countof(wPhpPathAndDllPath), L"LoadModule php7_module \"%ls/%ls\"\r\n", phpAbsBaseDir, phpApacheDllFileName);
        WToM(wPhpPathAndDllPath, phpPathAndDllPath, sizeof(phpPathAndDllPath));

        modify_conf_line_InsertOrUpdate_utf8AndAscii(apacheHttpdConFilePath, moduleContent, phpPathAndDllPath);

    }
    else if (versionNumber == 8) {
        if (moduleContent[0] == '\0') {
            strcpy_s(moduleContent, sizeof(moduleContent), "LoadModule php_module");
        }

        swprintf_s(wPhpPathAndDllPath, _countof(wPhpPathAndDllPath), L"LoadModule php_module \"%ls/%ls\"\r\n", phpAbsBaseDir, phpApacheDllFileName);
        WToM(wPhpPathAndDllPath, phpPathAndDllPath, sizeof(phpPathAndDllPath));

        modify_conf_line_InsertOrUpdate_utf8AndAscii(apacheHttpdConFilePath, moduleContent, phpPathAndDllPath);
    }

    if (!find_string_in_file_s(apacheHttpdConFilePath, "AddType application/x-httpd-php")) {
        modify_conf_line_InsertOrUpdate_utf8AndAscii(apacheHttpdConFilePath,
            "AddType application/x-httpd-php .php", "AddType application/x-httpd-php .php\r\n");
    }

    char phpFullPathStr[256];
    sprintf_s(phpFullPathStr, sizeof(phpFullPathStr), "PHPIniDir \"%ls\"\r\n", phpAbsBaseDir);

    modify_conf_line_InsertOrUpdate_utf8AndAscii(apacheHttpdConFilePath, "PHPIniDir", phpFullPathStr);
}

void SyncConfigFile(SoftwareGroupInfo softwareGroupInfo) {
    wchar_t* wProgramDirectory = get_current_program_directory_with_forward_slash();

    wchar_t webDir[256];
    swprintf_s(webDir, _countof(webDir), L"%ls/%ls", wProgramDirectory, DIRECTORY_WEB);
    if (!DirectoryExists(webDir)) {
        if (!CreateDirectory(webDir, NULL)) {
            return;
        }
    }

    wchar_t webDirDefault[256];
    swprintf_s(webDirDefault, _countof(webDirDefault), L"%ls/%ls", wProgramDirectory, DIRECTORY_WEB_DEFAULT);
    if (!DirectoryExists(webDirDefault)) {
        if (!CreateDirectory(webDirDefault, NULL)) {
            return;
        }
    }

    wchar_t webDefaultIndexFile[256];
    swprintf_s(webDefaultIndexFile, _countof(webDefaultIndexFile), L"%ls/index.php", webDirDefault);
    if (!CheckFileExists(webDefaultIndexFile)) {
        FILE* file;
        _wfopen_s(&file, webDefaultIndexFile, L"w");
        if (file != NULL) {
            fwprintf_s(file, L"<?php  echo \"Hello World!\";?>\n");
            fclose(file);
        }
    }

    // php

    // mysql
     
    // apache
    if (softwareGroupInfo.apache.version != NULL && softwareGroupInfo.php.version != NULL) {
        if (wcsstr(softwareGroupInfo.php.version, L"nts") == NULL) {
            syncConfigFilePHPAndApache(softwareGroupInfo.php.version, softwareGroupInfo.apache.version);
        }
    }

    //nginx
}
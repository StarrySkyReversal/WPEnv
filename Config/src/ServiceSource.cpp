#include "framework.h"
#include "ServiceUse.h"
#include "ServiceSource.h"
#include "BaseFileOpt.h"
#include <stdio.h>
#include <jansson.h>
#include "WindowLayout.h"
#include "Common.h"
#include <direct.h>

char* ServiceSourceData;
json_t* ServiceSourceDataJson;

int GetLinkFromJSON(const char* version, const char* softwareName, json_t* jsonString, char* outputBuffer, int bufferSize) {
    if (jsonString == NULL || version == NULL || softwareName == NULL || outputBuffer == NULL) {
        return -1;
    }

    json_t* softwareArray = json_object_get(jsonString, softwareName);
    if (!softwareArray || !json_is_array(softwareArray)) {
        return -1;
    }

    size_t index;
    json_t* item;
    json_array_foreach(softwareArray, index, item) {
        const char* key;
        json_t* value;

        json_object_foreach(item, key, value) {
            if (strcmp(version, key) == 0 && json_is_string(value)) {
                const char* linkStr = json_string_value(value);
                strncpy_s(outputBuffer, bufferSize, linkStr, bufferSize - 1);
                outputBuffer[bufferSize - 1] = '\0'; // 确保字符串以 null 结尾
                return 0;
            }
        }
    }

    return -1;
}

void ResolveSoftwareInfo(SoftwareInfo* software, const char* version, const char* softwareType) {
    char linkBuffer[2048];

    GetLinkFromJSON(version, softwareType, ServiceSourceDataJson, linkBuffer, sizeof(linkBuffer) / sizeof(char));

    char* versionCopy;
    char* versionNumber;
    char* context = NULL;

    versionCopy = _strdup(version);
    versionNumber = strtok_s(versionCopy, "_", &context);

    software->serviceType = _strdup(softwareType);
    software->version = _strdup(version);
    software->versionNumber = _strdup(versionNumber);
    software->link = _strdup(linkBuffer);
    software->fileFullName = GetFileFullNameFromUrl(version, software->link);

    free(versionCopy);
}

void SetSoftwareInfo(SoftwareInfo* software, HWND hList, const char* softwareType) {
    LRESULT selectedIndex = SendMessageA(hList, LB_GETCURSEL, 0, 0);

    char version[256];
    SendMessageA(hList, LB_GETTEXT, selectedIndex, (LPARAM)version);

    if (selectedIndex != -1) {
        ResolveSoftwareInfo(software, version, softwareType);
    }
    else {
        software->serviceType = NULL;
        software->version = NULL;
        software->versionNumber = NULL;
        software->link = NULL;
        software->fileFullName = NULL;
    }
}

DWORD GetServiceVersionInfo(SoftwareGroupInfo* softwareGroupInfo) {
    SetSoftwareInfo(&(softwareGroupInfo->php), hListPHP, "php");
    SetSoftwareInfo(&(softwareGroupInfo->mysql), hListMySQL, "mysql");
    SetSoftwareInfo(&(softwareGroupInfo->apache), hListApache, "apache");
    SetSoftwareInfo(&(softwareGroupInfo->nginx), hListNginx, "nginx");

    return 0;
}

DWORD GetConfigViewVersionInfo(SoftwareGroupInfo* softwareGroupInfo, ServiceUseConfig* serviceUse) {
    ResolveSoftwareInfo(&(softwareGroupInfo->php), serviceUse->php, "php");
    ResolveSoftwareInfo(&(softwareGroupInfo->mysql), serviceUse->mysql, "mysql");

    if (strstr(serviceUse->webService, "httpd") != NULL) {
        ResolveSoftwareInfo(&(softwareGroupInfo->apache), serviceUse->webService, "apache");

        softwareGroupInfo->nginx.serviceType = NULL;
        softwareGroupInfo->nginx.version = NULL;
        softwareGroupInfo->nginx.versionNumber = NULL;
        softwareGroupInfo->nginx.link = NULL;
        softwareGroupInfo->nginx.fileFullName = NULL;
    }
    else {
        ResolveSoftwareInfo(&(softwareGroupInfo->nginx), serviceUse->webService, "nginx");

        softwareGroupInfo->apache.serviceType = NULL;
        softwareGroupInfo->apache.version = NULL;
        softwareGroupInfo->apache.versionNumber = NULL;
        softwareGroupInfo->apache.link = NULL;
        softwareGroupInfo->apache.fileFullName = NULL;
    }

    return 0;
}

// https://windows.php.net/downloads/releases/archives/
void InitializeServiceSource() {
    const char* jsonTxt = R"(
{
    "php": [

        {"php-8.4.0_nts-vs17-x64":"https://windows.php.net/downloads/releases/archives/php-8.4.0-nts-Win32-vs17-x64.zip"},
        {"php-8.4.0_nts-vs17-x86":"https://windows.php.net/downloads/releases/archives/php-8.4.0-nts-Win32-vs17-x86.zip"},
        {"php-8.4.0_ts-vs17-x64":"https://windows.php.net/downloads/releases/archives/php-8.4.0-Win32-vs17-x64.zip"},
        {"php-8.4.0_ts-vs17-x86":"https://windows.php.net/downloads/releases/archives/php-8.4.0-Win32-vs17-x86.zip"},

        {"php-8.3.0_nts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.3.0-nts-Win32-vs16-x64.zip"},
        {"php-8.3.0_nts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.3.0-nts-Win32-vs16-x86.zip"},
        {"php-8.3.0_ts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.3.0-Win32-vs16-x64.zip"},
        {"php-8.3.0_ts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.3.0-Win32-vs16-x86.zip"},

        {"php-8.2.0_nts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.2.0-nts-Win32-vs16-x64.zip"},
        {"php-8.2.0_nts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.2.0-nts-Win32-vs16-x86.zip"},
        {"php-8.2.0_ts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.2.0-Win32-vs16-x64.zip"},
        {"php-8.2.0_ts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.2.0-Win32-vs16-x86.zip"},

        {"php-8.1.0_nts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.1.0-nts-Win32-vs16-x64.zip"},
        {"php-8.1.0_nts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.1.0-nts-Win32-vs16-x86.zip"},
        {"php-8.1.0_ts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.1.0-Win32-vs16-x64.zip"},
        {"php-8.1.0_ts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.1.0-Win32-vs16-x86.zip"},

        {"php-8.0.0_nts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.0.0-nts-Win32-vs16-x64.zip"},
        {"php-8.0.0_nts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.0.0-nts-Win32-vs16-x86.zip"},
        {"php-8.0.0_ts-vs16-x64":"https://windows.php.net/downloads/releases/archives/php-8.0.0-Win32-vs16-x64.zip"},
        {"php-8.0.0_ts-vs16-x86":"https://windows.php.net/downloads/releases/archives/php-8.0.0-Win32-vs16-x86.zip"},

        {"php-7.3.0_nts-vc15-x64":"https://windows.php.net/downloads/releases/archives/php-7.3.0-nts-Win32-VC15-x64.zip"},
        {"php-7.3.0_nts-vc15-x86":"https://windows.php.net/downloads/releases/archives/php-7.3.0-nts-Win32-VC15-x86.zip"},
        {"php-7.3.0_ts-vc15-x64":"https://windows.php.net/downloads/releases/archives/php-7.3.0-Win32-VC15-x64.zip"},
        {"php-7.3.0_ts-vc15-x86":"https://windows.php.net/downloads/releases/archives/php-7.3.0-Win32-VC15-x86.zip"},

        {"php-7.2.0_nts-vc15-x64":"https://windows.php.net/downloads/releases/archives/php-7.2.0-nts-Win32-VC15-x64.zip"},
        {"php-7.2.0_nts-vc15-x86":"https://windows.php.net/downloads/releases/archives/php-7.2.0-nts-Win32-VC15-x86.zip"},
        {"php-7.2.0_ts-vc15-x64":"https://windows.php.net/downloads/releases/archives/php-7.2.0-Win32-VC15-x64.zip"},
        {"php-7.2.0_ts-vc15-x86":"https://windows.php.net/downloads/releases/archives/php-7.2.0-Win32-VC15-x86.zip"},

        {"php-7.1.0_nts-vc14-x64":"https://windows.php.net/downloads/releases/archives/php-7.1.0-nts-Win32-VC14-x64.zip"},
        {"php-7.1.0_nts-vc14-x86":"https://windows.php.net/downloads/releases/archives/php-7.1.0-nts-Win32-VC14-x86.zip"},
        {"php-7.1.0_ts-vc14-x64":"https://windows.php.net/downloads/releases/archives/php-7.1.0-Win32-VC14-x64.zip"},
        {"php-7.1.0_ts-vc14-x86":"https://windows.php.net/downloads/releases/archives/php-7.1.0-Win32-VC14-x86.zip"},

        {"php-7.0.0_nts-vc14-x64":"https://windows.php.net/downloads/releases/archives/php-7.0.0-nts-Win32-VC14-x64.zip"},
        {"php-7.0.0_nts-vc14-x86":"https://windows.php.net/downloads/releases/archives/php-7.0.0-nts-Win32-VC14-x86.zip"},
        {"php-7.0.0_ts-vc14-x64":"https://windows.php.net/downloads/releases/archives/php-7.0.0-Win32-VC14-x64.zip"},
        {"php-7.0.0_ts-vc14-x86":"https://windows.php.net/downloads/releases/archives/php-7.0.0-Win32-VC14-x86.zip"},

        {"php-5.6.0_nts-vc11-x64":"https://windows.php.net/downloads/releases/archives/php-5.6.0-nts-Win32-VC11-x64.zip"},
        {"php-5.6.0_nts-vc11-x86":"https://windows.php.net/downloads/releases/archives/php-5.6.0-nts-Win32-VC11-x86.zip"},
        {"php-5.6.0_ts-vc11-x64":"https://windows.php.net/downloads/releases/archives/php-5.6.0-Win32-VC11-x64.zip"},
        {"php-5.6.0_ts-vc11-x86":"https://windows.php.net/downloads/releases/archives/php-5.6.0-Win32-VC11-x86.zip"},

        {"php-5.5.0_nts-vc11-x64":"https://windows.php.net/downloads/releases/archives/php-5.5.0-nts-Win32-VC11-x64.zip"},
        {"php-5.5.0_nts-vc11-x86":"https://windows.php.net/downloads/releases/archives/php-5.5.0-nts-Win32-VC11-x86.zip"},
        {"php-5.5.0_ts-vc11-x64":"https://windows.php.net/downloads/releases/archives/php-5.5.0-Win32-VC11-x64.zip"},
        {"php-5.5.0_ts-vc11-x86":"https://windows.php.net/downloads/releases/archives/php-5.5.0-Win32-VC11-x86.zip"}
    ],
    "mysql": [
        {"mysql-8.4.4-winx64":"https://dev.mysql.com/get/Downloads/MySQL-8.4/mysql-8.4.4-winx64.zip"},
        {"mysql-5.7.43-x64":"https://downloads.mysql.com/archives/get/p/23/file/mysql-5.7.43-winx64.zip"},
        {"mysql-5.7.43-x86":"https://downloads.mysql.com/archives/get/p/23/file/mysql-5.7.43-win32.zip"}

    ],
    "apache": [
        {"httpd-2.4.58_vs17-x64":"https://www.apachelounge.com/download/VS17/binaries/httpd-2.4.58-win64-VS17.zip"},
        {"httpd-2.4.58_vs17-x86":"https://www.apachelounge.com/download/vs17/binaries/httpd-2.4.58-win32-vs17.zip"}
    ],
    "nginx": [
        {"nginx-1.24.0":"http://nginx.org/download/nginx-1.24.0.zip"}
    ]
}
)";

    if (!DirectoryExists(DIRECTORY_CONFIG)) {
        if (_mkdir(DIRECTORY_CONFIG) != 0) {
            return;
        }
    }

    if (CheckFileExists(FILE_CONFIG_SERVICE_SOURCE)) {
        return;
    }

    FILE* file;

    // file exists
    errno_t err = fopen_s(&file, FILE_CONFIG_SERVICE_SOURCE, "w+N");
    if (err != 0 || !file) {
        if (file) {
            fclose(file);
        }

        return;
    }

    fprintf(file, "%s\n", jsonTxt);
    fclose(file);
}

void LoadServiceSourceData() {
    FILE* fp;
    fopen_s(&fp, FILE_CONFIG_SERVICE_SOURCE, "rb+N");
    if (!fp) {
        return;
    }

    fseek(fp, 0, SEEK_END);
    size_t length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (length <= 0) {
        fclose(fp);
        return;
    }

    char* buffer = (char*)malloc(length + 1);
    if (!buffer) {
        fclose(fp);
        return;
    }

    size_t readSize = fread_s(buffer, length + 1, sizeof(char), length, fp);
    fclose(fp);
    if (readSize >= length + 1) {
        free(buffer);
        return;
    }

    buffer[readSize] = '\0';

    json_error_t error;
    json_t* root = json_loads(buffer, 0, &error);

    if (!root) {
        OutputDebugStringA(error.text);

        return;
    }

    ServiceSourceData = buffer;

    // We can't free here. If we do, the value of GlobalServiceRepositoryOriginal will be lost.
    // free(buffer);
    json_decref(root);
}

void LoadServiceSourceDataToListBox() {
    json_t* item;
    json_error_t error;

    ServiceSourceDataJson = json_loads(ServiceSourceData, 0, &error);

    if (!ServiceSourceDataJson) {
        return;
    }

    const char* sections[] = { "php", "mysql", "nginx", "apache" };
    HWND hLists[] = { hListPHP, hListMySQL, hListNginx, hListApache };

    for (int i = 0; i < 4; i++) {
        item = json_object_get(ServiceSourceDataJson, sections[i]);
        if (!json_is_array(item)) {
            continue;
        }

        size_t index;
        json_t* value;
        json_array_foreach(item, index, value) {
            const char* key;
            json_t* subvalue;

            json_object_foreach(value, key, subvalue) {
                SendMessageA(hLists[i], LB_ADDSTRING, 0, (LPARAM)key);
            }
        }
    }
}
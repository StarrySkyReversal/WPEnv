#include "framework.h"
#include "ServiceSource.h"
#include "BaseFileOpt.h"
#include <stdio.h>
#include <jansson.h>
#include "WindowLayout.h"
#include "Common.h"

char* ServiceSourceData;
json_t* ServiceSourceDataJson;

const wchar_t* GetLinkFromJSON(const wchar_t* version, const char* softwareName, json_t* jsonString, wchar_t* outputBuffer, int bufferSize) {
    char wVersion[1024];
    WToM(version, wVersion, sizeof(wVersion));

    if (json_object_get(jsonString, softwareName)) {
        json_t* softwareArray = json_object_get(jsonString, softwareName);
        size_t index;
        json_t* item;

        json_array_foreach(softwareArray, index, item) {
            const char* key;
            json_t* value;

            json_object_foreach(item, key, value) {
                if (strcmp(wVersion, key) == 0) {
                    const char* linkStr = json_string_value(value);

                    MToW(linkStr, outputBuffer, bufferSize);

                    return outputBuffer;
                }
            }
        }
    }

    return L"";
}

void SetSoftwareInfo(SoftwareInfo* software, HWND hList, const char* softwareType, json_t* json) {
    LRESULT selectedIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);

    wchar_t version[256];
    SendMessage(hList, LB_GETTEXT, selectedIndex, (LPARAM)version);

    if (selectedIndex != -1) {
        wchar_t linkBuffer[2048];

        GetLinkFromJSON(version, softwareType, json, linkBuffer, sizeof(linkBuffer) / sizeof(wchar_t));

        wchar_t wServiceType[32];
        MToW(softwareType, wServiceType, _countof(wServiceType));

        software->serviceType = _wcsdup(wServiceType);
        software->version = _wcsdup(version);
        software->link = _wcsdup(linkBuffer);
        software->fileFullName = GetFileFullNameFromUrl(version, software->link);
    }
    else {
        software->serviceType = NULL;
        software->version = NULL;
        software->link = NULL;
        software->fileFullName = NULL;
    }
}

DWORD GetServiceVersionInfo(SoftwareGroupInfo* softwareGroupInfo) {
    SetSoftwareInfo(&(softwareGroupInfo->php), hListPHP, "php", ServiceSourceDataJson);
    SetSoftwareInfo(&(softwareGroupInfo->mysql), hListMySQL, "mysql", ServiceSourceDataJson);
    SetSoftwareInfo(&(softwareGroupInfo->apache), hListApache, "apache", ServiceSourceDataJson);
    SetSoftwareInfo(&(softwareGroupInfo->nginx), hListNginx, "nginx", ServiceSourceDataJson);

    return 1;
}

// https://windows.php.net/downloads/releases/archives/
void InitializeServiceSource() {
    const wchar_t* jsonTxt = LR"(
{
    "php": [
        {"php-8.2.6-nts":"https://windows.php.net/downloads/releases/archives/php-8.2.6-nts-Win32-vs16-x64.zip"},
        {"php-8.2.6":"https://windows.php.net/downloads/releases/archives/php-8.2.6-Win32-vs16-x64.zip"},
        {"php-8.2.1-nts":"https://windows.php.net/downloads/releases/archives/php-8.2.1-nts-Win32-vs16-x64.zip"},
        {"php-8.2.1":"https://windows.php.net/downloads/releases/archives/php-8.2.1-Win32-vs16-x64.zip"},
        {"php-8.0.1-nts":"https://windows.php.net/downloads/releases/archives/php-8.0.1-nts-Win32-vs16-x64.zip"},
        {"php-8.0.1":"https://windows.php.net/downloads/releases/archives/php-8.0.1-Win32-vs16-x64.zip"},
        {"php-7.3.1-nts":"https://windows.php.net/downloads/releases/archives/php-7.3.1-nts-Win32-VC15-x64.zip"},
        {"php-7.3.1":"https://windows.php.net/downloads/releases/archives/php-7.3.1-Win32-VC15-x64.zip"},
        {"php-7.0.1-nts":"https://windows.php.net/downloads/releases/archives/php-7.0.1-nts-Win32-VC14-x64.zip"},
        {"php-7.0.1":"https://windows.php.net/downloads/releases/archives/php-7.0.1-Win32-VC14-x64.zip"},
        {"php-5.6.35-nts":"https://windows.php.net/downloads/releases/archives/php-5.6.35-nts-Win32-VC11-x64.zip"},
        {"php-5.6.35":"https://windows.php.net/downloads/releases/archives/php-5.6.35-Win32-VC11-x64.zip"},
        {"php-5.5.1-nts":"https://windows.php.net/downloads/releases/archives/php-5.5.1-nts-Win32-VC11-x64.zip"},
        {"php-5.5.1":"https://windows.php.net/downloads/releases/archives/php-5.5.1-Win32-VC11-x64.zip"}
    ],
    "mysql": [
        {"mysql-8.0.33":"https://downloads.mysql.com/archives/get/p/23/file/mysql-8.0.33-winx64.zip"},
        {"mysql-5.7.42":"https://downloads.mysql.com/archives/get/p/23/file/mysql-5.7.42-winx64.zip"},
        {"mysql-5.6.10":"https://downloads.mysql.com/archives/get/p/23/file/mysql-5.6.10-winx64.zip"},
        {"mysql-5.5.8":"https://downloads.mysql.com/archives/get/p/23/file/mysql-5.5.8-winx64.zip"}
    ],
    "apache": [
        {"httpd-2.4.57":"https://www.apachelounge.com/download/VS17/binaries/httpd-2.4.57-win64-VS17.zip"}
    ],
    "nginx": [
        {"nginx-1.24.0":"http://nginx.org/download/nginx-1.24.0.zip"},
        {"nginx-1.22.1":"http://nginx.org/download/nginx-1.22.1.zip"}
    ]
}
)";


    if (!DirectoryExists(DIRECTORY_CONFIG)) {
        if (_wmkdir(DIRECTORY_CONFIG) != 0) {
            return;
        }
    }

    if (CheckFileExists(FILE_CONFIG_SERVICE_SOURCE)) {
        return;
    }

    FILE* file;

    // file exists
    errno_t err = _wfopen_s(&file, FILE_CONFIG_SERVICE_SOURCE, L"w");
    if (err != 0 || !file) {
        if (file) {
            fclose(file);
        }

        return;
    }

    fwprintf(file, L"%ls\n", jsonTxt);
    fclose(file);
}

void LoadServiceSourceData() {
    FILE* fp;
    _wfopen_s(&fp, FILE_CONFIG_SERVICE_SOURCE, L"rb");
    if (!fp) {
        return;
    }

    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
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
                int len = MultiByteToWideChar(CP_UTF8, 0, key, -1, NULL, 0);
                wchar_t* wkey = new wchar_t[len];
                MultiByteToWideChar(CP_UTF8, 0, key, -1, wkey, len);

                SendMessage(hLists[i], LB_ADDSTRING, 0, (LPARAM)wkey);

                delete[] wkey;
            }
        }
    }
}
#include "framework.h"
#include "ServiceSource.h"
#include "BaseFileOpt.h"
#include <stdio.h>
#include <jansson.h>
#include "WindowLayout.h"
#include "Common.h"
#include<direct.h>

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

void SetSoftwareInfo(SoftwareInfo* software, HWND hList, const char* softwareType, json_t* json) {
    LRESULT selectedIndex = SendMessageA(hList, LB_GETCURSEL, 0, 0);

    char version[256];
    SendMessageA(hList, LB_GETTEXT, selectedIndex, (LPARAM)version);

    if (selectedIndex != -1) {
        char linkBuffer[2048];

        GetLinkFromJSON(version, softwareType, json, linkBuffer, sizeof(linkBuffer) / sizeof(char));

        software->serviceType = _strdup(softwareType);
        software->version = _strdup(version);
        software->link = _strdup(linkBuffer);
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
    const char* jsonTxt = R"(
{
    "php": [
        {"php-8.3.0":"https://windows.php.net/downloads/releases/archives/php-8.3.0-Win32-vs16-x64.zip"}
    ],
    "mysql": [
        {"mysql-5.7.43":"https://downloads.mysql.com/archives/get/p/23/file/mysql-5.7.43-winx64.zip"}
    ],
    "apache": [
        {"httpd-2.4.58":"https://www.apachelounge.com/download/VS17/binaries/httpd-2.4.58-win64-VS17.zip"}
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
    errno_t err = fopen_s(&file, FILE_CONFIG_SERVICE_SOURCE, "w");
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
    fopen_s(&fp, FILE_CONFIG_SERVICE_SOURCE, "rb");
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
                SendMessageA(hLists[i], LB_ADDSTRING, 0, (LPARAM)key);
            }
        }
    }
}
#pragma once

#include "ServiceSource.h"

#define MAX_CONFIGS 256
#define MAX_CONFIG_LEN 256

typedef struct {
    char php[MAX_CONFIG_LEN];
    char mysql[MAX_CONFIG_LEN];
    char webService[MAX_CONFIG_LEN];
    int itemCount;
} ServiceUseConfig;

int ReadServiceUseConfig(const char* filePath, ServiceUseConfig* configs);
int AddConfigToServiceUse(const char* filePath, const char* config);

void LoadServiceUseDataToListView();

bool AddNewConfig(SoftwareGroupInfo softwareGroupInfo);

void RemoveListViewSelectedItem();

bool GetServiceUseItem(ServiceUseConfig* config);

void getApacheVersionAbsBaseDir(const char* version, char* buffer, int bufferSize);


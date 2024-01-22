#pragma once

#include "ServiceSource.h"

#define MAX_CONFIGS 256
#define MAX_CONFIG_LEN 256

typedef struct {
    wchar_t php[MAX_CONFIG_LEN];
    wchar_t mysql[MAX_CONFIG_LEN];
    wchar_t webService[MAX_CONFIG_LEN];
    int itemCount;
} ServiceUseConfig;

int ReadServiceUseConfig(const wchar_t* filePath, ServiceUseConfig* configs);
int AddConfigToServiceUse(const wchar_t* filePath, const wchar_t* config);

void LoadServiceUseDataToListView();

bool AddNewConfig(SoftwareGroupInfo softwareGroupInfo);
void RemoveListViewSelectedItem();

bool GetServiceUseItem(ServiceUseConfig* config);

void InitializeApacheConfigFile(SoftwareInfo softwareInfo);

void InitializeNginxConfigFile(SoftwareInfo softwareInfo);

void SyncConfigFile(SoftwareGroupInfo softwareGroupInfo);

void syncConfigFilePHPAndApache(const wchar_t* phpVersion, const wchar_t* apacheVersion);

void syncPHPConfigFile(SoftwareInfo softwareInfo);

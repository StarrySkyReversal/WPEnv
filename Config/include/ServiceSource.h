#pragma once

#include "ConfigHeader.h"

void InitializeServiceSource();
void LoadServiceSourceData();
void LoadServiceSourceDataToListBox();
DWORD GetServiceVersionInfo(SoftwareGroupInfo* softwareGroupInfo);
void ResolveSoftwareInfo(SoftwareInfo* software, const char* version, const char* softwareType);
DWORD GetConfigViewVersionInfo(SoftwareGroupInfo* softwareGroupInfo, ServiceUseConfig* serviceUse);
bool AddNewConfig(SoftwareGroupInfo softwareGroupInfo);
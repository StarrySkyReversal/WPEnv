#pragma once

#include "ConfigHeader.h"

int ReadServiceUseConfig(const char* filePath, ServiceUseConfig* configs);
int AddConfigToServiceUse(const char* filePath, const char* config);

void LoadServiceUseDataToListView();

bool AddNewConfig(SoftwareGroupInfo softwareGroupInfo);

void RemoveListViewSelectedItem();

bool GetServiceUseItem(ServiceUseConfig* config);

void getApacheVersionAbsBaseDir(const char* version, char* buffer, int bufferSize);


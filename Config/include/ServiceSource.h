#pragma once

typedef struct {
    const char* version;
    const char* link;
    const char* serviceType;
    const char* fileFullName;
} SoftwareInfo;

typedef struct {
    SoftwareInfo php;
    SoftwareInfo mysql;
    SoftwareInfo apache;
    SoftwareInfo nginx;
} SoftwareGroupInfo;

void InitializeServiceSource();
void LoadServiceSourceData();
void LoadServiceSourceDataToListBox();
DWORD GetServiceVersionInfo(SoftwareGroupInfo* softwareGroupInfo);

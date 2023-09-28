#pragma once

typedef struct {
    const wchar_t* version;
    const wchar_t* link;
    const wchar_t* serviceType;
    const wchar_t* fileFullName;
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

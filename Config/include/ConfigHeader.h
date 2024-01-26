#pragma once

typedef struct {
    const char* version;
    const char* versionNumber;
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

typedef struct {
    char php[256];
    char mysql[256];
    char webService[256];
    int itemCount;
} ServiceUseConfig;

void FreeSoftwareInfo(SoftwareInfo* info);

void FreeSoftwareGroupInfo(SoftwareGroupInfo* group);

SoftwareInfo DeepCopySoftwareInfo(const SoftwareInfo* source, const char* serviceType);

SoftwareGroupInfo* DeepCopySoftwareGroupInfo(const SoftwareGroupInfo* source);
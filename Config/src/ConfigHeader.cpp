#include "ConfigHeader.h"
#include "stdlib.h"
#include "string.h"

void FreeSoftwareInfo(SoftwareInfo* info) {
    if (info->version != NULL && strcmp(info->version, "") != 0) {
        free((void*)info->serviceType);
        free((void*)info->version);
        free((void*)info->versionNumber);
        free((void*)info->link);
        free((void*)info->fileFullName);
    }
}

void FreeSoftwareGroupInfo(SoftwareGroupInfo* group) {
    if (!group) return;
    FreeSoftwareInfo(&group->php);
    FreeSoftwareInfo(&group->mysql);
    FreeSoftwareInfo(&group->apache);
    FreeSoftwareInfo(&group->nginx);

    delete group;
}

SoftwareInfo DeepCopySoftwareInfo(const SoftwareInfo* source, const char* serviceType) {
    SoftwareInfo dest;
    if (source->version != NULL) {
        dest.serviceType = _strdup(serviceType);
        dest.version = _strdup(source->version);
        dest.versionNumber = _strdup(source->versionNumber);
        dest.link = _strdup(source->link);
        dest.fileFullName = _strdup(source->fileFullName);
    }
    else {
        dest.serviceType = "";
        dest.version = "";
        dest.versionNumber = "";
        dest.link = "";
        dest.fileFullName = "";
    }

    return dest;
}

SoftwareGroupInfo* DeepCopySoftwareGroupInfo(const SoftwareGroupInfo* source) {
    SoftwareGroupInfo* dest = new SoftwareGroupInfo;

    dest->php = DeepCopySoftwareInfo(&source->php, "php");
    dest->mysql = DeepCopySoftwareInfo(&source->mysql, "mysql");
    dest->apache = DeepCopySoftwareInfo(&source->apache, "apache");
    dest->nginx = DeepCopySoftwareInfo(&source->nginx, "nginx");

    return dest;
}
#pragma once

#include "Resource.h"
#include "ServiceSource.h"

extern HWND hWndMain;
extern CRITICAL_SECTION daemonMonitorServiceCs;

typedef struct {
    bool bRun;

    const char* phpExe;
    const char* phpExePath;
    const char* phpExeDirectory;
    const char* phpConfDirectory;
    const char* phpIniFile;
    const char* phpVersion;

    const char* mysqldExe;
    const char* mysqldExePath;
    const char* mysqldExeDirectory;
    const char* mysqldConfDirectory;
    const char* mysqldVersion;

    const char* webServiceExe;
    const char* webServiceExePath;
    const char* webServiceExeDirectory;
    const char* webServiceConfDirectory;
    const char* webServiceVhostsFile;
    const char* webServiceVersion;
} WebDaemonService;

DWORD StartDaemonService(bool bWait = false);
DWORD StopDaemonService(bool bWait = false);
void RestartDaemonService();

DWORD WINAPI DaemonMonitorService(LPVOID lParam);

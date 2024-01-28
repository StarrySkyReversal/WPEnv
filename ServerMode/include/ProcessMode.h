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

    const char* mysqldExe;
    const char* mysqldExePath;
    const char* mysqldExeDirectory;

    const char* webServiceExe;
    const char* webServiceExePath;
    const char* webServiceExeDirectory;
} WebDaemonService;

DWORD StartDaemonService();
void CloseDaemonService();
void RestartDaemonService();

DWORD DaemonMonitorService(LPVOID lParam);

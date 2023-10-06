#pragma once

#include "Resource.h"

extern HWND hWndMain;
extern CRITICAL_SECTION daemonMonitorServiceCs;

DWORD StartDaemonService();
void CloseDaemonService();
void RestartDaemonService();

DWORD DaemonMonitorService(LPVOID lParam);

#pragma once

#include "Resource.h"

extern HWND hWndMain;


DWORD StartDaemonService();
void CloseDaemonService();
void RestartDaemonService();

DWORD DaemonMonitorService(LPVOID lParam);

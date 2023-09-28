#pragma once

#include "Resource.h"

const UINT WM_APP_TRAYMSG = WM_APP + 1;
const UINT ID_TRAY_ICON = 1;
const UINT ID_MENU_EXIT = 1001;

void SetupTrayIcon(HWND hwnd, NOTIFYICONDATA* nid);

void MinimizeToTray(HWND hwnd);

void HandleTrayMessage(HWND hwnd, LPARAM lParam);

void RemoveTrayIcon(NOTIFYICONDATA* nid);

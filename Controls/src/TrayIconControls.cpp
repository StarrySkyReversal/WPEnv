#include "framework.h"
#include "stdio.h"
#include <shellapi.h>
#include "TrayIconControls.h"
#include "IniOpt.h"

#pragma comment(lib, "shell32.lib")

#define ID_MENU_OPEN_FOLDER_VERSION 2001
#define ID_MENU_OPEN_FOLDER_CONFIG 2002
#define ID_MENU_OPEN_FOLDER_DOWNLOAD 2003
#define ID_MENU_OPEN_FOLDER_SERVICE 2004
#define ID_MENU_OPEN_FOLDER_CONF_DIR 2005
#define ID_MENU_OPEN_FOLDER_VHOSTS_FILE 2006

void SetupTrayIcon(HWND hwnd, NOTIFYICONDATA* nid) {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    nid->cbSize = sizeof(*nid);
    nid->hWnd = hwnd;
    nid->uID = ID_TRAY_ICON;
    nid->uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid->uCallbackMessage = WM_APP_TRAYMSG;
    nid->hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
    wcscpy_s(nid->szTip, L"My App");
    Shell_NotifyIcon(NIM_ADD, nid);
}

void MinimizeToTray(HWND hwnd) {
    ShowWindow(hwnd, SW_HIDE);
}

void HandleTrayMessage(HWND hwnd, LPARAM lParam) {
    switch (lParam) {
        case WM_LBUTTONUP:
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            break;
        case WM_RBUTTONUP:
        {
            char ServiceConfDirectory[512];
            char ServiceVhostsFile[512];
            char ServiceVersion[512];
            char ServiceVersionTemp[512];
            if (webDaemonServiceInstance.webServiceVersion != NULL) {
                strcpy_s(ServiceVersionTemp, webDaemonServiceInstance.webServiceVersion);
            }
            else {
                read_ini_file("config/base.ini", "Service", "Version", ServiceVersionTemp, sizeof(ServiceVersionTemp));
            }
            sprintf_s(ServiceVersion, sizeof(ServiceVersion), "Version[%s]", ServiceVersionTemp);

            if (webDaemonServiceInstance.webServiceConfDirectory != NULL) {
                strcpy_s(ServiceConfDirectory, webDaemonServiceInstance.webServiceConfDirectory);
            }
            else {
                read_ini_file("config/base.ini", "Service", "LastConfDir", ServiceConfDirectory, sizeof(ServiceConfDirectory));
            }
            
            if(webDaemonServiceInstance.webServiceVhostsFile != NULL) {
                strcpy_s(ServiceVhostsFile, webDaemonServiceInstance.webServiceVhostsFile);
            }
            else {
                read_ini_file("config/base.ini", "Service", "LastVhostDir", ServiceVhostsFile, sizeof(ServiceVhostsFile));
            }

            HMENU hMenu = CreatePopupMenu();
            AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, ServiceVersion);
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_CONFIG, "Config dir");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_DOWNLOAD, "Downloads dir");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_CONF_DIR, "Service conf dir");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_VHOSTS_FILE, "Service vhosts file");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_EXIT, "Exit");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);  // Make sure the menu closes properly
            int selectedId = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);

            switch (selectedId) {
            case ID_MENU_OPEN_FOLDER_VERSION:
                ShellExecuteA(hwnd, "", ServiceVersion, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_CONFIG:
                ShellExecuteA(hwnd, "open", DIRECTORY_CONFIG, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_DOWNLOAD:
                ShellExecuteA(hwnd, "open", DIRECTORY_DOWNLOAD, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_CONF_DIR:
                ShellExecuteA(hwnd, "open", ServiceConfDirectory, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_VHOSTS_FILE:
                ShellExecuteA(hwnd, "open", ServiceVhostsFile, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_EXIT:
                PostQuitMessage(0);
                break;
            default:
                break;
            }

            DestroyMenu(hMenu);
            //DeleteObject(hBoldFont);
        }
        break;
    }
}

void RemoveTrayIcon(NOTIFYICONDATA* nid) {
    Shell_NotifyIcon(NIM_DELETE, nid);
}

#include "framework.h"
#include <shellapi.h>
#include "TrayIconControls.h"

#pragma comment(lib, "shell32.lib")

#define ID_MENU_OPEN_FOLDER_CONFIG 2001
#define ID_MENU_OPEN_FOLDER_DOWNLOAD 2002
#define ID_MENU_OPEN_FOLDER_SERVICE 2003

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
            HMENU hMenu = CreatePopupMenu();
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_CONFIG, "Config Directory");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_DOWNLOAD, "Downloads Directory");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_EXIT, "Exit");

            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);  // Make sure the menu closes properly
            int selectedId = TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);

            switch (selectedId) {
            case ID_MENU_OPEN_FOLDER_CONFIG:
                ShellExecuteA(hwnd, "open", DIRECTORY_CONFIG, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_DOWNLOAD:
                ShellExecuteA(hwnd, "open", DIRECTORY_DOWNLOAD, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_SERVICE:
                ShellExecuteA(hwnd, "open", DIRECTORY_SERVICE, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_EXIT:
                PostQuitMessage(0);
                break;
            default:
                break;
            }

            DestroyMenu(hMenu);
        }
        break;
    }
}

void RemoveTrayIcon(NOTIFYICONDATA* nid) {
    Shell_NotifyIcon(NIM_DELETE, nid);
}

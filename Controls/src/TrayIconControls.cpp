#include "framework.h"
#include "stdio.h"
#include <shellapi.h>
#include "TrayIconControls.h"
#include "IniOpt.h"

#pragma comment(lib, "shell32.lib")

#define ID_MENU_OPEN_FOLDER_VERSION 2001
#define ID_MENU_OPEN_FOLDER_CONFIG 2002
#define ID_MENU_OPEN_FOLDER_DOWNLOAD 2003

#define ID_MENU_OPEN_FOLDER_SERVER_CONF 2005

#define ID_MENU_OPEN_FILE_SERVER_VHOSTS 2006

#define ID_MENU_OPEN_FOLDER_PHP 2007
#define ID_MENU_OPEN_FILE_PHP_INI 2008

#define ID_MENU_OPEN_FOLDER_MYSQL 2009

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


            AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, "---------WPEnv---------");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_CONFIG, "WPEnv Config dir");
            AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_DOWNLOAD, "WPEnv Downloads dir");

            // WebServer
            char ServiceConfDirectory[512] = { '\0' };
            char ServiceVhostsFile[512] = { '\0' };
            char ServiceVersion[512] = { '\0' };
            char ServiceVersionTemp[512] = { '\0' };
            if (webDaemonServiceInstance.webServiceVersion != NULL) {
                strcpy_s(ServiceVersionTemp, webDaemonServiceInstance.webServiceVersion);
            }
            else {
                read_ini_file("config/base.ini", "Service", "Version", ServiceVersionTemp, sizeof(ServiceVersionTemp));
            }

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

            if (strcmp(ServiceVersionTemp, "") != 0) {
                AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, "-------------------------");

                sprintf_s(ServiceVersion, sizeof(ServiceVersion), "Server Version[%s]", ServiceVersionTemp);
                AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, ServiceVersion);
            }

            if (strcmp(ServiceConfDirectory, "") != 0) {
                AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_SERVER_CONF, "Service conf dir");
            }
            if (strcmp(ServiceVhostsFile, "") != 0) {
                AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FILE_SERVER_VHOSTS, "Service vhosts file");
            }

            // PHP
            char phpConfDirectory[512] = { '\0' };
            char phpIniFile[512] = { '\0' };
            char phpVersion[512] = { '\0' };
            char phpVersionTemp[512] = { '\0' };

            if (webDaemonServiceInstance.phpVersion != NULL) {
                strcpy_s(phpVersionTemp, webDaemonServiceInstance.phpVersion);
            }
            else {
                read_ini_file("config/base.ini", "PHP", "Version", phpVersionTemp, sizeof(phpVersionTemp));
            }
            if (webDaemonServiceInstance.phpConfDirectory != NULL) {
                strcpy_s(phpConfDirectory, webDaemonServiceInstance.phpConfDirectory);
            }
            else {
                read_ini_file("config/base.ini", "PHP", "LastConfDir", phpConfDirectory, sizeof(phpConfDirectory));
            }
            if (webDaemonServiceInstance.phpIniFile != NULL) {
                strcpy_s(phpIniFile, webDaemonServiceInstance.phpIniFile);
            }
            else {
                read_ini_file("config/base.ini", "PHP", "LastPHPIni", phpIniFile, sizeof(phpIniFile));
            }

            if (strcmp(phpVersionTemp, "") != 0) {
                AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, "-------------------------");

                sprintf_s(phpVersion, sizeof(phpVersion), "PHP Version[%s]", phpVersionTemp);
                AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, phpVersion);
            }

            if (strcmp(phpConfDirectory, "") != 0) {
                AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_PHP, "PHP conf dir");
            }

            if (strcmp(phpIniFile, "") != 0) {
                AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FILE_PHP_INI, "php.ini");
            }
        
            // MySQL
            char mysqldConfDirectory[512] = { '\0' };
            char mysqlVersion[512] = { '\0' };
            char mysqlVersionTemp[512] = { '\0' };

            if (webDaemonServiceInstance.mysqldVersion != NULL) {
                strcpy_s(mysqlVersionTemp, webDaemonServiceInstance.mysqldVersion);
            }
            else {
                read_ini_file("config/base.ini", "Mysql", "Version", mysqlVersionTemp, sizeof(mysqlVersionTemp));
            }
            if (webDaemonServiceInstance.mysqldConfDirectory != NULL) {
                strcpy_s(mysqldConfDirectory, webDaemonServiceInstance.mysqldConfDirectory);
            }
            else {
                read_ini_file("config/base.ini", "Mysql", "LastConfDir", mysqldConfDirectory, sizeof(mysqldConfDirectory));
            }

            if (strcmp(mysqlVersionTemp, "") != 0) {
                AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, "-------------------------");
                sprintf_s(mysqlVersion, sizeof(mysqlVersion), "Mysql Version[%s]", mysqlVersionTemp);
                AppendMenuA(hMenu, MF_STRING | MF_DISABLED, ID_MENU_OPEN_FOLDER_VERSION, mysqlVersion);
            }

            if (strcmp(mysqldConfDirectory, "") != 0) {
                AppendMenuA(hMenu, MF_STRING, ID_MENU_OPEN_FOLDER_MYSQL, "Mysql conf dir");
            }

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
            case ID_MENU_OPEN_FOLDER_SERVER_CONF:
                ShellExecuteA(hwnd, "open", ServiceConfDirectory, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FILE_SERVER_VHOSTS:
                ShellExecuteA(hwnd, "open", ServiceVhostsFile, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_PHP:
                ShellExecuteA(hwnd, "open", phpConfDirectory, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FILE_PHP_INI:
                ShellExecuteA(hwnd, "open", phpIniFile, NULL, NULL, SW_SHOWDEFAULT);
                break;
            case ID_MENU_OPEN_FOLDER_MYSQL:
                ShellExecuteA(hwnd, "open", mysqldConfDirectory, NULL, NULL, SW_SHOWDEFAULT);
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

// WPEnv.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "WPEnv.h"
#include "ListBoxControls.h"
#include "RichEditControls.h"
#include "ProgressBarControls.h"
#include "StaticLabelControls.h"
#include "ListViewControls.h"
#include "ButtonControls.h"
#include "WindowAdaptive.h"
#include "ServiceSource.h"
#include "ServiceUse.h"
#include "FontStyle.h"
#include "DownloadCenter.h"
#include "ProcessMode.h"
#include "WindowLayout.h"
#include <commctrl.h>
#include <shellapi.h>
#include "TrayIconControls.h"
#include "ModeMonitor.h"

#define MAX_LOADSTRING 100

// Global Window Handle For Main
HWND hWndMain;
CRITICAL_SECTION daemonMonitorServiceCs;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

bool IsRunAsAdmin() {
    BOOL isRunAsAdmin = FALSE;
    PSID adminGroupSid = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    // Allocate and initialize a SID of the administrators group.
    if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroupSid)) {
        return false;
    }

    // Check whether the SID of administrators group is enabled in the primary access token of the process.
    if (!CheckTokenMembership(NULL, adminGroupSid, &isRunAsAdmin)) {
        isRunAsAdmin = FALSE;
    }

    // Free the SID now.
    if (adminGroupSid) {
        FreeSid(adminGroupSid);
    }

    return (isRunAsAdmin == TRUE);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    if (!IsRunAsAdmin()) {
        wchar_t szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
            // Launch itself as admin
            SHELLEXECUTEINFO sei = { sizeof(sei) };
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;
            if (!ShellExecuteEx(&sei)) {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_CANCELLED) {
                    MessageBox(NULL, L"App requires admin permissions!", L"Error", MB_OK);
                }
            }
            return 1; // Exit the current process
        }
    }

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WPENV, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WPENV));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WPENV));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WPENV);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
const int WINDOW_WIDTH = 655;
const int WINDOW_HEIGHT = 525;
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   RECT rc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
   AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

   int width = rc.right - rc.left;
   int height = rc.bottom - rc.top;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, width, height, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static NOTIFYICONDATA nid = {};

    switch (message)
    {
    case WM_CREATE:
    {
        hWndMain = hWnd;

        CreateWindowFont();

        InitializeRichEditLibrary();

        CreateListBoxControls(hWnd, hInst);
        CreateRichEditControls(hWnd, hInst);
        CreateProgressBarControls(hWnd, hInst);
        CreateStaticLabelControls(hWnd, hInst);
        CreateListViewControls(hWnd, hInst);
        CreateButtonControls(hWnd, hInst);

        InitializeControlsAdaptive(hWnd);

        InitializeServiceSource();
        LoadServiceSourceData();

        LoadServiceSourceDataToListBox();
        LoadServiceUseDataToListView();

        EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP_SERVICE), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_RESTART_SERVICE), FALSE);

        SetupTrayIcon(hWnd, &nid);

        SetWindowFont(hWnd);
    }
    break;
    case WM_SHOWWINDOW:
    {
        InitializeCriticalSection(&daemonMonitorServiceCs);

        HANDLE daemonService = CreateThread(NULL, 0, DaemonMonitorService, NULL, 0, NULL);
        if (daemonService) {
            CloseHandle(daemonService);
        }
    }
    break;
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        SetControlsAdaptive(width, height);
    }
    break;
    case WM_COMMAND:
        {

        int wmId = LOWORD(wParam);

        SoftwareGroupInfo softwareGroupInfo;

        // Parse the menu selections:
        switch (wmId)
        {
        case IDC_BUTTON_DOWNLOAD:
        {
            GetServiceVersionInfo(&softwareGroupInfo);

            StartDownload(softwareGroupInfo);
        }
        break;
        case IDC_BUTTON_ADD_CONFIG:
        {
            GetServiceVersionInfo(&softwareGroupInfo);

            AddNewConfig(softwareGroupInfo);
            SyncConfigFile(softwareGroupInfo);
        }

            break;
        case IDC_BUTTON_REMOVE_CONFIG:
            RemoveListViewSelectedItem();
            break;
        case IDC_BUTTON_START_SERVICE:
            StartDaemonService();

            break;
        case IDC_BUTTON_STOP_SERVICE:
            CloseDaemonService();

            break;
        case IDC_BUTTON_RESTART_SERVICE:
            RestartDaemonService();

            break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_MENU_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        }
        break;
    //case WM_ERASEBKGND:
    //    return 1;
    case WM_PAINT:
        {
            if ((HWND)lParam == hStaticLabel) {
                SendMessage(hStaticLabel, WM_PRINT, NULL, PRF_CHILDREN | PRF_CLIENT | PRF_ERASEBKGND | PRF_NONCLIENT);

                return 0;
            }

            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...

            EndPaint(hWnd, &ps);
        }
        break;
    //case WM_CTLCOLORSTATIC:
    //    if ((HWND)lParam == hStaticLabel)
    //    {
    //        return SendMessage(hStaticLabel, WM_CTLCOLORSTATIC, wParam, lParam);
    //    }
    //    break;
    case WM_DRAWITEM:
    {
        SendMessage((HWND)((LPDRAWITEMSTRUCT)lParam)->hwndItem, message, wParam, lParam);

        //return 0;
    }
    break;

    case WM_MEASUREITEM:
    {
        LPMEASUREITEMSTRUCT lpMeasureItem = (LPMEASUREITEMSTRUCT)lParam;
        if (lpMeasureItem->CtlType == ODT_LISTVIEW && lpMeasureItem->CtlID == IDC_LISTBOX_CONFIG) {
            lpMeasureItem->itemHeight = 20;

            //return 0;
        }
    }
    break;
    //case WM_ERASEBKGND:
    //    return 1;
    //case WM_ERASEBKGND:
    //{
    //    HDC hdc = (HDC)wParam;
    //    RECT rc;
    //    GetClientRect(hWnd, &rc);
    //    FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW + 1));
    //    return 1;
    //}
    //break;
    //case WM_POWERBROADCAST:
    //    if (wParam == PBT_APMRESUMEAUTOMATIC)
    //    {
    //        InvalidateRect(hWnd, NULL, TRUE);  // Marking the entire window area as invalid will result in WM_ PAINT message sent
    //    }
    //    break;
    case WM_APP_TRAYMSG:
        HandleTrayMessage(hWnd, lParam);
        return 0;
    case WM_CLOSE:
        MinimizeToTray(hWnd);
        break;
    case WM_DESTROY:
        DeleteCriticalSection(&daemonMonitorServiceCs);

        FreeControlRectArray();
        FreeRichEdit();
        RemoveTrayIcon(&nid);
        CleanupFont();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

#include "framework.h"
#include "WindowLayout.h"
#include "ButtonControls.h"
#include "ServiceSource.h"
#include "DownloadCenter.h"
#include "ServiceUse.h"
#include "ProcessMode.h"

#include <commctrl.h>

#define PROP_IS_INITIALIZED    TEXT("IsInitialized")
#define PROP_CURRENT_STATE     TEXT("CurrentState")
#define PROP_IS_TRACKING       TEXT("IsTracking")

// Define the state of the button
typedef enum {
    BUTTON_STATE_DEFAULT,
    BUTTON_STATE_HOVER,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_DISABLED,
} BUTTON_STATE;

void SetInitialButtonState(HWND hWnd, BUTTON_STATE* state) {
    if (IsWindowEnabled(hWnd)) {
        *state = BUTTON_STATE_DEFAULT;
    }
    else {
        *state = BUTTON_STATE_DISABLED;
    }
}

LRESULT CALLBACK CustomButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    BUTTON_STATE currentState = (BUTTON_STATE)(LONG_PTR)GetProp(hWnd, PROP_CURRENT_STATE);
    BOOL isTracking = (BOOL)(LONG_PTR)GetProp(hWnd, PROP_IS_TRACKING);

    if (GetProp(hWnd, PROP_IS_INITIALIZED) == NULL) {
        SetInitialButtonState(hWnd, &currentState);
        SetProp(hWnd, PROP_IS_INITIALIZED, (HANDLE)TRUE);
        SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);
        SetProp(hWnd, PROP_IS_TRACKING, (HANDLE)FALSE);
    }


    switch (uMsg)
    {
    case WM_MOUSEMOVE:
        if (IsWindowEnabled(hWnd)) {
            if (!isTracking) {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
                isTracking = TRUE;
                SetProp(hWnd, PROP_IS_TRACKING, (HANDLE)TRUE);
            }

            if (currentState != BUTTON_STATE_PRESSED) {
                currentState = BUTTON_STATE_HOVER;
                SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }
        break;
    case WM_MOUSELEAVE:
        if (IsWindowEnabled(hWnd)) {
            currentState = BUTTON_STATE_DEFAULT;
            InvalidateRect(hWnd, NULL, TRUE);
            isTracking = FALSE;
            SetProp(hWnd, PROP_IS_TRACKING, (HANDLE)FALSE);
            SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        if (IsWindowEnabled(hWnd)) {
            SetCapture(hWnd);
            currentState = BUTTON_STATE_PRESSED;
            InvalidateRect(hWnd, NULL, TRUE);

            SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (IsWindowEnabled(hWnd)) {
            ReleaseCapture();

            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);

            RECT rc;
            GetClientRect(hWnd, &rc);

            if (PtInRect(&rc, pt)) {
                currentState = BUTTON_STATE_DEFAULT;
                InvalidateRect(hWnd, NULL, TRUE);

                SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);

                // Send a WM_COMMAND message to the parent window.
                PostMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
            }
            else {
                // Release the cursor outside the button area.
                currentState = BUTTON_STATE_DEFAULT;
                InvalidateRect(hWnd, NULL, TRUE);
                SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);
            }

            return 0;
        }

        break;
    case WM_LBUTTONDBLCLK:
        return 0;

    case WM_KILLFOCUS:
        if (IsWindowEnabled(hWnd)) {
            currentState = BUTTON_STATE_DEFAULT;
            InvalidateRect(hWnd, NULL, TRUE);

            SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);
            return 0;
        }

        break;
    case WM_CREATE: // Set the initial state when the button is created.
        if (IsWindowEnabled(hWnd)) {
            currentState = BUTTON_STATE_DEFAULT;
        }
        else {
            currentState = BUTTON_STATE_DISABLED;
        }
        break;
    case WM_ENABLE:
        if (wParam == FALSE) {
            currentState = BUTTON_STATE_DISABLED;
        }
        else {
            currentState = BUTTON_STATE_DEFAULT;
            SetProp(hWnd, PROP_IS_TRACKING, (HANDLE)FALSE);
        }

        SetProp(hWnd, PROP_CURRENT_STATE, (HANDLE)currentState);

        InvalidateRect(hWnd, NULL, TRUE);

        return 0;
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // Create an off-screen DC and bitmap
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);

        // Select the bitmap into the off-screen DC
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        // Draw the button based on the currentState variable.
        switch (currentState)
        {
        case BUTTON_STATE_PRESSED:
            {
                HBRUSH hBrush = CreateSolidBrush(RGB(30, 90, 150));
                FillRect(hdcMem, &rc, hBrush);
                DeleteObject(hBrush);
            }
            break;

        case BUTTON_STATE_HOVER:
            {
                HBRUSH hBrush = CreateSolidBrush(RGB(70, 160, 245));
                FillRect(hdcMem, &rc, hBrush);
                DeleteObject(hBrush);
            }
            break;

        case BUTTON_STATE_DISABLED:
            {
                HBRUSH hBrush = CreateSolidBrush(RGB(190, 190, 190));
                FillRect(hdcMem, &rc, hBrush);
                DeleteObject(hBrush);
            }
            break;

            default:
            {
                HBRUSH hBrush = CreateSolidBrush(RGB(50, 140, 230));
                FillRect(hdcMem, &rc, hBrush);
                DeleteObject(hBrush);
            }
            break;
        }

        TCHAR szText[100];
        GetWindowText(hWnd, szText, sizeof(szText) / sizeof(TCHAR));
        SetTextColor(hdcMem, RGB(255, 255, 255));
        SetBkMode(hdcMem, TRANSPARENT);
        SelectObject(hdcMem, hFont);
        DrawText(hdcMem, szText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        // Copy the off-screen bitmap onto the screen
        BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hdcMem, 0, 0, SRCCOPY);

        // Clean up
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);

        return 0;
    }
    break;
    case WM_ERASEBKGND:
    {
        return 0;
    }
    case WM_NCDESTROY:
    {
        RemoveWindowSubclass(hWnd, CustomButtonSubclassProc, uIdSubclass);

        RemoveProp(hWnd, PROP_IS_INITIALIZED);
        RemoveProp(hWnd, PROP_CURRENT_STATE);
        RemoveProp(hWnd, PROP_IS_TRACKING);
    }
    break;

    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CreateButtonControls(HWND hWnd, HINSTANCE hInstance) {
    hButtonDownload = CreateWindow(
        L"BUTTON",
        L"\u2193",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50 + 4 * (buttonWidth + spacing * 7),
        30,
        buttonWidth,
        buttonHeight,
        hWnd,
        (HMENU)IDC_BUTTON_DOWNLOAD,
        hInstance,
        nullptr);

    hButtonAddConfig = CreateWindow(
        L"BUTTON",
        L"\u2795",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50 + 4 * (buttonWidth + spacing * 7),
        80,
        buttonWidth,
        buttonHeight,
        hWnd,
        (HMENU)IDC_BUTTON_ADD_CONFIG,
        hInstance,
        nullptr);

    hButtonStart = CreateWindow(
        L"BUTTON",
        L"\u25B6",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50 + 4 * (buttonWidth + spacing * 7),
        0 + listBoxPlusSpacing + 20,
        buttonWidth,
        buttonHeight,
        hWnd,
        (HMENU)IDC_BUTTON_START_SERVICE,
        hInstance,
        nullptr);

    hButtonStop = CreateWindow(
        L"BUTTON",
        L"\u23F9",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50 + 4 * (buttonWidth + spacing * 7),
        0 + listBoxPlusSpacing + 70,
        buttonWidth,
        buttonHeight,
        hWnd,
        (HMENU)IDC_BUTTON_STOP_SERVICE,
        hInstance,
        nullptr);

    hButtonRestart = CreateWindow(
        L"BUTTON",
        L"\U0001F504",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50 + 4 * (buttonWidth + spacing * 7),
        0 + listBoxPlusSpacing + 120,
        buttonWidth,
        buttonHeight,
        hWnd,
        (HMENU)IDC_BUTTON_RESTART_SERVICE,
        hInstance,
        nullptr);


    hButtonRemove = CreateWindow(
        L"BUTTON",
        L"\u274C",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        50 + 4 * (buttonWidth + spacing * 7),
        0 + listBoxPlusSpacing + 170,
        buttonWidth,
        buttonHeight,
        hWnd,
        (HMENU)IDC_BUTTON_REMOVE_CONFIG,
        hInstance,
        nullptr);

    SetWindowSubclass(hButtonDownload, CustomButtonSubclassProc, 0, 0);
    SetWindowSubclass(hButtonAddConfig, CustomButtonSubclassProc, 0, 0);
    SetWindowSubclass(hButtonStart, CustomButtonSubclassProc, 0, 0);
    SetWindowSubclass(hButtonStop, CustomButtonSubclassProc, 0, 0);
    SetWindowSubclass(hButtonRestart, CustomButtonSubclassProc, 0, 0);
    SetWindowSubclass(hButtonRemove, CustomButtonSubclassProc, 0, 0);
}
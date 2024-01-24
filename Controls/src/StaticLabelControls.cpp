#include "framework.h"
#include "StaticLabelControls.h"
#include "WindowLayout.h"
#include <commctrl.h>

#define PROP_BRUSH TEXT("MY_STATIC_PROP_BRUSH")

LRESULT CALLBACK StaticSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    //case WM_CTLCOLORSTATIC:
    //{
    //    HBRUSH hBrushStaticBkgnd = (HBRUSH)GetProp(hWnd, PROP_BRUSH);

    //    if (!hBrushStaticBkgnd)
    //    {
    //        hBrushStaticBkgnd = CreateSolidBrush(RGB(255, 255, 255));
    //        SetProp(hWnd, PROP_BRUSH, (HANDLE)hBrushStaticBkgnd);
    //    }

    //    HDC hdcStatic = (HDC)wParam;
    //    SetBkColor(hdcStatic, RGB(255, 255, 255));

    //    return (INT_PTR)hBrushStaticBkgnd;
    //}
    case WM_PAINT:
    //case WM_PRINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        if (PRF_CLIENT)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);

            SelectObject(hdc, hFont);

            TCHAR szText[100];
            GetWindowText(hWnd, szText, sizeof(szText) / sizeof(TCHAR));
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        //if (PRF_NONCLIENT)
        //{
        //    RECT rc;
        //    GetWindowRect(hWnd, &rc);
        //    OffsetRect(&rc, -rc.left, -rc.top);

        //    HBRUSH hBrushBorder = CreateSolidBrush(RGB(233, 233, 233));
        //    FrameRect(hdc, &rc, hBrushBorder);
        //    DeleteObject(hBrushBorder);
        //}

        EndPaint(hWnd, &ps);

        ReleaseDC(hWnd, hdc);

        return 0;
    }
    case WM_NCDESTROY:
        HBRUSH hBrushStaticBkgnd = (HBRUSH)GetProp(hWnd, PROP_BRUSH);
        if (hBrushStaticBkgnd)
        {
            DeleteObject(hBrushStaticBkgnd);
            RemoveProp(hWnd, PROP_BRUSH);
        }

        RemoveWindowSubclass(hWnd, StaticSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CreateStaticLabelControls(HWND hWnd, HINSTANCE hInstance) {
    hStaticLabel = CreateWindow(
        L"STATIC",
        L"",
        WS_CHILD | WS_VISIBLE,
        30,
        listBoxHeight * 2 + 160,
        listBoxWidth * 4 + spacing * 3,
        20,
        hWnd,
        (HMENU)IDC_PROGRESS_INFO,
        hInstance,
        nullptr);

    SetWindowSubclass(hStaticLabel, StaticSubclassProc, IDC_PROGRESS_INFO, 0);
}

void UpdateStaticLabelInfo(const char* String) {
    SendMessage(hStaticLabel, WM_SETREDRAW, FALSE, 0);

    SendMessageA(hStaticLabel, WM_SETTEXT, 0, (LPARAM)String);

    SendMessage(hStaticLabel, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hStaticLabel, NULL, TRUE);
}

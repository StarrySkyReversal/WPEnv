#include "framework.h"
#include "WindowLayout.h"
#include "ProgressBarControls.h"
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

LRESULT CALLBACK SubclassedProgressBarProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    static int border_thickness = 1;
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rc;
        GetClientRect(hWnd, &rc);

        LRESULT rangeMax = SendMessage(hWnd, PBM_GETRANGE, FALSE, 0);
        LRESULT pos = SendMessage(hWnd, PBM_GETPOS, 0, 0);
        int barWidth = MulDiv(rc.right, (int)pos, (int)rangeMax);

        FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

        rc.right = barWidth;
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 156, 255));  // Green color
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_NCPAINT:
    {
        //HDC hdc = GetWindowDC(hWnd);
        //RECT rc;
        //GetClientRect(hWnd, &rc);
        //rc.right += 2 * border_thickness + 1;
        //rc.bottom += 2 * border_thickness + 1;

        //HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        //HPEN hpen = CreatePen(PS_SOLID, 2 * border_thickness, RGB(233, 233, 233));
        //HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, hbrush);
        //HPEN oldpen = (HPEN)SelectObject(hdc, hpen);
        //Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        //SelectObject(hdc, oldpen);
        //SelectObject(hdc, oldbrush);
        //DeleteObject(hpen);
        //DeleteObject(hbrush);

        //ReleaseDC(hWnd, hdc);
        return 0;
    }
    case WM_ERASEBKGND:
    {
        return 0;
    }
    case WM_NCDESTROY:
    {
        RemoveWindowSubclass(hWnd, SubclassedProgressBarProc, uIdSubclass);
        break;
    }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CreateProgressBarControls(HWND hWnd, HINSTANCE hInstance) {
    hProgressBar = CreateWindowEx(0,
        PROGRESS_CLASS,
        NULL,
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        30,
        //listBoxHeight * 2 + 5,
        listBoxHeight * 2 + 155,
        listBoxWidth * 4 + spacing * 3,
        5,
        hWnd,
        (HMENU)IDC_PROGRESS,
        hInstance,
        NULL);

    SendMessage(hProgressBar, PBM_SETRANGE32, 0, 100);
    SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

    SetWindowSubclass(hProgressBar, SubclassedProgressBarProc, 0, 0);
}

void SetProgressBarPosition(int positionValue) {
    SendMessage(hProgressBar, PBM_SETPOS, positionValue, 0);
}

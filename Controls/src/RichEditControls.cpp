#include "framework.h"
#include "WindowLayout.h"
#include "RichEditControls.h"
#include <richedit.h>
#include <time.h>
#include "Log.h"

#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")

HMODULE hMsftedit;

LRESULT CALLBACK RichEditSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    static int border_thickness = 1;
    switch (uMsg) {
    case WM_NCPAINT:
    {
        HDC hdc = GetWindowDC(hWnd);
        RECT rc;
        GetClientRect(hWnd, &rc);
        rc.right += 2 * border_thickness + 1;
        rc.bottom += 2 * border_thickness + 1;

        HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HPEN hpen = CreatePen(PS_SOLID, 2 * border_thickness, RGB(216, 216, 216));
        HBRUSH oldbrush = (HBRUSH)SelectObject(hdc, hbrush);
        HPEN oldpen = (HPEN)SelectObject(hdc, hpen);
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, oldpen);
        SelectObject(hdc, oldbrush);
        DeleteObject(hpen);
        DeleteObject(hbrush);

        ReleaseDC(hWnd, hdc);
        return 0;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, RichEditSubClassProc, 0);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void InitializeRichEditLibrary() {
    hMsftedit = LoadLibrary(L"Msftedit.dll");
    if (!hMsftedit) {
        LogAndMsgBox(L"Failed to load Msftedit.dll");
    }
}

void FreeRichEdit() {
    DestroyWindow(hRichEdit);
    FreeLibrary(hMsftedit);
}

void CreateRichEditControls(HWND hWnd, HINSTANCE hInstance) {
    hRichEdit = CreateWindowEx(
        WS_EX_STATICEDGE,
        MSFTEDIT_CLASS,  // Use MSFTEDIT_CLASS instead of 'EDIT'.
        L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
        30,
        listBoxHeight * 2 - 10,
        listBoxWidth * 4 + spacing * 3,
        160,
        hWnd,
        (HMENU)IDC_RICH_EDIT,
        hInstance,
        nullptr);

    SetWindowSubclass(GetDlgItem(hWnd, IDC_RICH_EDIT), RichEditSubClassProc, 0, 0);
}

void getCurrentDateTime(char* buffer, size_t bufferSize) {
    time_t t;
    struct tm tm_info;

    time(&t);
    localtime_s(&tm_info, &t);

    //wcsftime(buffer, bufferSize, L"%Y-%m-%d %H:%M:%S  ", &tm_info);
    strftime(buffer, bufferSize, "%H:%M:%S ", &tm_info);
}

void ClearRichEdit() {
    SetWindowText(hRichEdit, L"");
}

void LogOutput(const char* type, const char* content) {
    int nLength = GetWindowTextLength(hRichEdit);
    SendMessageA(hRichEdit, EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);

    /////////////////////////////////////

    char datetime[128];
    getCurrentDateTime(datetime, sizeof(datetime));

    CHARFORMAT cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    //cf.dwMask = CFM_SIZE;
    //cf.yHeight = 10 * 20;

    // time
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(136, 136, 158);
    SendMessageA(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    SendMessageA(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)datetime);

    // info or error
    //cf.dwMask = CFM_COLOR;
    //cf.dwEffects = CFE_BOLD;
    if (type == "ERR") {
        cf.crTextColor = RGB(238, 96, 96);
    }
    else if (type == "WARN") {
        cf.crTextColor = RGB(255, 150, 60);
    }
    else {
        cf.crTextColor = RGB(10, 138, 50);
    }
    SendMessageA(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    char typeStr[256] = { 0 };
    sprintf_s(typeStr, sizeof(typeStr), "%s ", type);
    SendMessageA(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)typeStr);

    // content
    //cf.dwMask = CFM_COLOR;
    //cf.dwEffects &= ~CFE_BOLD;
    cf.crTextColor = RGB(86, 86, 86);
    SendMessageA(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    SendMessageA(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)content);

    LRESULT nLinesCount = SendMessage(hRichEdit, EM_GETLINECOUNT, 0, (LPARAM)0);
    SendMessageA(hRichEdit, EM_SCROLL, SB_LINEDOWN, nLinesCount + 1);
}

void InfoOutput(const char* str) {
    LogOutput("INFO", str);
}

void WarnOutput(const char* str) {
    LogOutput("WARN", str);
}

void ErrOutput(const char* str) {
    LogOutput("ERR", str);
}

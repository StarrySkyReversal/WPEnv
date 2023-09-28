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

void CheckRichEditLinesForErrors(HWND hRichEdit) {
    LRESULT lineCount = SendMessage(hRichEdit, EM_GETLINECOUNT, 0, 0);
    for (int i = 0; i < lineCount; i++) {
        wchar_t lineText[1024];
        *(WORD*)lineText = sizeof(lineText);
        LRESULT len = SendMessage(hRichEdit, EM_GETLINE, i, (LPARAM)lineText);
        lineText[len] = L'\0';

        if (wcsncmp(lineText, L"ERROR", 5) == 0) {
            CHARFORMAT cf;
            memset(&cf, 0, sizeof(cf));
            cf.cbSize = sizeof(cf);
            cf.dwMask = CFM_COLOR;
            cf.crTextColor = RGB(255, 0, 0);

            LRESULT lineStart = SendMessage(hRichEdit, EM_LINEINDEX, i, 0);
            LRESULT lineEnd = lineStart + len;

            SendMessage(hRichEdit, EM_SETSEL, lineStart, lineEnd);
            SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        }
    }

    SendMessage(hRichEdit, EM_SETSEL, -1, -1);
}

void getCurrentDateTime(wchar_t* buffer, size_t bufferSize) {
    time_t t;
    struct tm tm_info;

    time(&t);
    localtime_s(&tm_info, &t);

    //wcsftime(buffer, bufferSize, L"%Y-%m-%d %H:%M:%S  ", &tm_info);
    wcsftime(buffer, bufferSize, L"%H:%M:%S ", &tm_info);
}

void ClearRichEdit() {
    SetWindowText(hRichEdit, L"");
}

void AppendEditInfo(const wchar_t* string) {
    // Get the text length of the edit box so we can determine where to insert new content.
    int nLength = GetWindowTextLength(hRichEdit);

    // Move the insertion point to the end of the text.
    SendMessage(hRichEdit, EM_SETSEL, (WPARAM)nLength, (LPARAM)nLength);

    CHARFORMAT cfSize;
    ZeroMemory(&cfSize, sizeof(cfSize));

    cfSize.cbSize = sizeof(cfSize);
    cfSize.dwMask = CFM_SIZE; // We want to modify font size
    cfSize.yHeight = 10 * 20; // font size is in twips (1/1440 inch or 1/20 of a point)

    wchar_t datetime[128];
    getCurrentDateTime(datetime, sizeof(datetime) / sizeof(wchar_t));

    wchar_t combinedStr[2048];
    swprintf_s(combinedStr, _countof(combinedStr), L"%s %s", datetime, string);

    // Define a gray CHARFORMAT.
    CHARFORMAT cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = RGB(136, 136, 136);

    // Set the format first.
    SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    // Then insert date and time text.
    SendMessage(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)datetime);

    // Reset the format to default.
    cf.crTextColor = RGB(51, 51, 51);
    SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    // Insert new content.
    SendMessage(hRichEdit, EM_REPLACESEL, FALSE, (LPARAM)string);

    LRESULT iLines = SendMessage(hRichEdit, EM_GETLINECOUNT, 0, 0);
    SendMessage(hRichEdit, EM_SCROLL, SB_LINEDOWN, iLines + 1);

    SendMessage(hRichEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cfSize);

    CheckRichEditLinesForErrors(hRichEdit);
}

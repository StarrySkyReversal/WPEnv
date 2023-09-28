#include "framework.h"
#include "WindowLayout.h"
#include "FontStyle.h"

void CreateWindowFont() {
    if (hFont) {
        DeleteObject(hFont);
    }

    hFont = CreateFont(
        16,
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Arial"
    );
}

BOOL CALLBACK EnumChildProcSetFont(HWND hwndChild, LPARAM lParam) {
    HFONT hFont = (HFONT)lParam;
    SendMessage(hwndChild, WM_SETFONT, (WPARAM)hFont, TRUE);
    return TRUE;
}

void SetWindowFont(HWND hWnd) {
    EnumChildWindows(hWnd, EnumChildProcSetFont, (LPARAM)hFont);
}

void CleanupFont() {
    if (hFont) {
        DeleteObject(hFont);
        hFont = NULL;
    }
}

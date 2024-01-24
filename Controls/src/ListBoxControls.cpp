#include "framework.h"
#include "WindowLayout.h"
#include "ListBoxControls.h"
#include "BaseFileOpt.h"

#include <commctrl.h>

LRESULT CALLBACK SubclassedListBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS->CtlType == ODT_LISTBOX)
        {
            HDC hdc = pDIS->hDC;
            RECT rcItem = pDIS->rcItem;

            if (pDIS->itemID == -1) break;

            char buffer[256];
            LRESULT numDataLength = SendMessageA(pDIS->hwndItem, LB_GETTEXT, pDIS->itemID, (LPARAM)buffer);
            if (numDataLength <= 0) {
                break;
            }

            bool isDisabled = !(IsWindowEnabled(pDIS->hwndItem));

            FileList* fileList = NULL;
            bool bFileExists = CheckDownloadFileExists(buffer, &fileList);
            FreeCheckDownloadFileExists(fileList);

            if (isDisabled) {
                SetTextColor(hdc, RGB(160, 160, 160));
                SetBkColor(hdc, RGB(240, 240, 240));
            }
            else if (pDIS->itemState & ODS_SELECTED)
            {
                SetTextColor(hdc, RGB(255, 255, 255));
                SetBkColor(hdc, RGB(0, 123, 255));
            }
            else
            {
                if (bFileExists) {
                    SetTextColor(hdc, RGB(34, 139, 34));
                    SetBkColor(hdc, RGB(212, 244, 215));
                }
                else {
                    SetTextColor(hdc, RGB(51, 51, 51));
                    SetBkColor(hdc, RGB(255, 255, 255));
                }
            }

            HBRUSH hBrush = CreateSolidBrush(GetBkColor(hdc));
            FillRect(hdc, &rcItem, hBrush);
            DeleteObject(hBrush);

            //if (wcslen(buffer) > 0) {
            if (buffer[0] != L'\0') {
                DrawTextA(hdc, buffer, -1, &rcItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            return 0;
        }

    }
    break;
    case WM_NCPAINT:
    {
        HDC hdc;
        RECT rc;

        GetWindowRect(hWnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);

        hdc = GetWindowDC(hWnd);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(216, 216, 216));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);

        DeleteObject(hPen);
        ReleaseDC(hWnd, hdc);

        return 0;
    }
    case WM_LBUTTONDOWN:
        {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };

            LRESULT clickedIndex = SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
            LRESULT selectedIndex = SendMessage(hWnd, LB_GETCURSEL, 0, 0);

            if (clickedIndex == selectedIndex && selectedIndex != LB_ERR) {
                PostMessage(hWnd, LB_SETCURSEL, (WPARAM)-1, 0);
            }
        }
        break;
    case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, SubclassedListBoxProc, uIdSubclass); 
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CreateListBoxControls(HWND hWnd, HINSTANCE hInstance) {
    // PHP ListBox
    hListPHP = CreateWindow(
        L"LISTBOX",
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY,
        30,
        30,
        listBoxWidth,
        listBoxHeight,
        hWnd,
        (HMENU)IDC_LISTBOX_PHP,
        hInstance,
        nullptr);

    // MySQL ListBox
    hListMySQL = CreateWindowA(
        "LISTBOX",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY,
        30 + listBoxWidth + spacing,
        30,
        listBoxWidth,
        listBoxHeight,
        hWnd,
        (HMENU)IDC_LISTBOX_MYSQL,
        hInstance,
        nullptr);


    // Apache ListBox
    hListApache = CreateWindowA(
        "LISTBOX",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY,
        30 + 2 * (listBoxWidth + spacing),
        30,
        listBoxWidth,
        listBoxHeight,
        hWnd,
        (HMENU)IDC_LISTBOX_APACHE,
        hInstance,
        nullptr);

    // Nginx ListBox
    hListNginx = CreateWindowA(
        "LISTBOX",
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY,
        30 + 3 * (listBoxWidth + spacing),
        30,
        listBoxWidth,
        listBoxHeight,
        hWnd,
        (HMENU)IDC_LISTBOX_NGINX,
        hInstance,
        nullptr);

    SetWindowSubclass(GetDlgItem(hWnd, IDC_LISTBOX_PHP), SubclassedListBoxProc, 0, 0);
    SetWindowSubclass(GetDlgItem(hWnd, IDC_LISTBOX_MYSQL), SubclassedListBoxProc, 0, 0);
    SetWindowSubclass(GetDlgItem(hWnd, IDC_LISTBOX_APACHE), SubclassedListBoxProc, 0, 0);
    SetWindowSubclass(GetDlgItem(hWnd, IDC_LISTBOX_NGINX), SubclassedListBoxProc, 0, 0);

    SendMessage(hListPHP, LB_SETITEMHEIGHT, 0, MAKELPARAM(20, 0));
    SendMessage(hListMySQL, LB_SETITEMHEIGHT, 0, MAKELPARAM(20, 0));
    SendMessage(hListApache, LB_SETITEMHEIGHT, 0, MAKELPARAM(20, 0));
    SendMessage(hListNginx, LB_SETITEMHEIGHT, 0, MAKELPARAM(20, 0));
}
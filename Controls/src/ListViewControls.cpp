#include "framework.h"
#include <richedit.h>
#include <commctrl.h>
#include "WindowLayout.h"
#include "ListViewControls.h"
#include "Log.h"
#include "Common.h"

#pragma comment(lib, "Comctl32.lib")

LRESULT CALLBACK ListViewSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {

    switch (uMsg)
    {
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
        RECT rcItem = lpDrawItem->rcItem;

        int iSelectedIndex = ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);

        if (IsWindowEnabled(hWnd)) {

            if (lpDrawItem->itemState & ODS_SELECTED)
            {
                SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            }
            else
            {
                SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
                SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, GetSysColorBrush(COLOR_WINDOW));
            }
        }
        else {
            if (lpDrawItem->itemID == iSelectedIndex)
            {
                SetTextColor(lpDrawItem->hDC, RGB(0, 0, 0));
                SetBkColor(lpDrawItem->hDC, RGB(173, 216, 230));
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, CreateSolidBrush(RGB(173, 216, 230)));
            }
            else
            {
                SetTextColor(lpDrawItem->hDC, RGB(150, 150, 150));
                SetBkColor(lpDrawItem->hDC, RGB(220, 220, 220));
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, CreateSolidBrush(RGB(220, 220, 220)));
            }
        }

        int iColumnCount = Header_GetItemCount(ListView_GetHeader(hWnd));
        for (int iColumn = 0; iColumn < iColumnCount; iColumn++)
        {
            RECT rcSubItem;
            ListView_GetSubItemRect(hWnd, lpDrawItem->itemID, iColumn, LVIR_LABEL, &rcSubItem);

            TCHAR szText[256];
            ListView_GetItemText(hWnd, lpDrawItem->itemID, iColumn, szText, sizeof(szText) / sizeof(TCHAR));

            SelectObject(lpDrawItem->hDC, hFont);

            if (iColumn == 0) {
                DrawText(lpDrawItem->hDC, szText, -1, &rcSubItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            else {
                rcSubItem.left += 30;  // Add a left margin of 10 pixels.
                DrawText(lpDrawItem->hDC, szText, -1, &rcSubItem, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

        }

        return TRUE;
    }
    break;
    case WM_NCPAINT:
    {
        HDC hdc;
        RECT rc;

        GetWindowRect(hWnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);

        hdc = GetWindowDC(hWnd);

        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

        SelectObject(hdc, hOldPen);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hFont);

        DeleteObject(hPen);
        ReleaseDC(hWnd, hdc);

        return 0;
    }
    break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, ListViewSubclassProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK CustomHeaderProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        int itemCount = Header_GetItemCount(hWnd);

        // Create a white pen for drawing vertical lines.
        HPEN hWhitePen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hWhitePen);

        SelectObject(hdc, hFont);

        // Iterate through each item and draw.
        for (int i = 0; i < itemCount; i++)
        {
            RECT rcItem;
            Header_GetItemRect(hWnd, i, &rcItem);

            HBRUSH hBrush = CreateSolidBrush(RGB(235, 245, 251));
            FillRect(hdc, &rcItem, hBrush);

            // Get the text of the header item.
            TCHAR szText[255];
            HDITEM hdi = { 0 };
            hdi.mask = HDI_TEXT;
            hdi.pszText = szText;
            hdi.cchTextMax = sizeof(szText) / sizeof(TCHAR);
            Header_GetItem(hWnd, i, &hdi);

            SetTextColor(hdc, RGB(44, 62, 80));
            // Set the background mode to transparent so that the text won't cover the background color.
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, szText, -1, &rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Draw a right separator line for all items except the last one.
            if (i != itemCount - 1)
            {
                MoveToEx(hdc, rcItem.right - 1, rcItem.top, NULL);
                LineTo(hdc, rcItem.right - 1, rcItem.bottom);
            }

            DeleteObject(hBrush);
        }

        // Restore the original pen and delete the white pen we created.
        SelectObject(hdc, hOldPen);
        DeleteObject(hWhitePen);

        EndPaint(hWnd, &ps);
        return 0;
    }
    break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, CustomHeaderProc, uIdSubclass);
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void AdjustListViewColumns(HWND hWndListView)
{
    int columnCount = Header_GetItemCount(ListView_GetHeader(hWndListView));

    int* colMaxWidths = (int*)malloc(columnCount * sizeof(int));
    if (colMaxWidths == NULL)
    {
        return;
    }
    memset(colMaxWidths, 0, columnCount * sizeof(int));

    RECT rcListView;
    GetClientRect(hWndListView, &rcListView);
    int totalListViewWidth = rcListView.right - rcListView.left;
    int totalColumnsWidth = 0;

    // Calculate the maximum content width for each column.
    for (int col = 0; col < columnCount; ++col)
    {
        TCHAR szText[256];
        LVCOLUMN lvCol = {};
        lvCol.mask = LVCF_TEXT;
        lvCol.pszText = szText;
        lvCol.cchTextMax = sizeof(szText) / sizeof(TCHAR);
        ListView_GetColumn(hWndListView, col, &lvCol);
        int headerWidth = ListView_GetStringWidth(hWndListView, szText);

        int itemCount = ListView_GetItemCount(hWndListView);
        for (int item = 0; item < itemCount; ++item)
        {
            ListView_GetItemText(hWndListView, item, col, szText, sizeof(szText) / sizeof(TCHAR));
            int itemWidth = ListView_GetStringWidth(hWndListView, szText) + 15;
            if (itemWidth > colMaxWidths[col])
                colMaxWidths[col] = itemWidth;
        }

        if (headerWidth > colMaxWidths[col])
            colMaxWidths[col] = headerWidth;

        totalColumnsWidth += colMaxWidths[col];
    }

    // Adjust column widths to fill the entire ListView.
    for (int col = 0; col < columnCount; ++col)
    {
        int adjustedWidth = (colMaxWidths[col] * totalListViewWidth) / totalColumnsWidth;
        ListView_SetColumnWidth(hWndListView, col, adjustedWidth);
    }

    free(colMaxWidths);
}

void CreateListViewControls(HWND hWnd, HINSTANCE hInstance) {
    hListConfig = CreateWindowEx(
        0,
        WC_LISTVIEW,
        L"List Config",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LVS_REPORT | WS_BORDER | LVS_SHOWSELALWAYS | LVS_OWNERDRAWFIXED | LVS_SORTASCENDING | LVS_SORTDESCENDING,
        30,
        0 + listBoxPlusSpacing + 20,
        listBoxWidth * 4 + spacing * 3,
        listBoxHeight - 60,
        hWnd,
        (HMENU)IDC_LISTBOX_CONFIG,
        hInstance,
        nullptr);

    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.pszText = (LPWSTR)L"No";
    ListView_InsertColumn(hListConfig, 0, &lvc);

    lvc.pszText = (LPWSTR)L"PHP";
    ListView_InsertColumn(hListConfig, 1, &lvc);

    lvc.pszText = (LPWSTR)L"Mysql";
    ListView_InsertColumn(hListConfig, 2, &lvc);

    lvc.pszText = (LPWSTR)L"Service";
    ListView_InsertColumn(hListConfig, 3, &lvc);

    ListView_SetExtendedListViewStyle(hListConfig, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    HWND hListViewHeader = ListView_GetHeader(hListConfig);
    SetWindowSubclass(hListViewHeader, CustomHeaderProc, 0, 0);

    SetWindowSubclass(hListConfig, ListViewSubclassProc, 0, 0);

    AdjustListViewColumns(hListConfig);
}

int GetListViewItemInsertIndex(HWND hWnd) {
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    return ListView_InsertItem(hWnd, &lvi);
}

int GetListViewCount(HWND hWnd) {
    return ListView_GetItemCount(hWnd);
}

void AddListViewItem(HWND hWnd, int itemIndex, int ColumnIndex, const char* text) {
    wchar_t* wText = new wchar_t[256];
    MToW(text, wText, 256);

    ListView_SetItemText(hWnd, itemIndex, ColumnIndex, wText);

    //delete[] wText;
}

int GetListViewSelectedIndex(HWND hWnd) {
    return ListView_GetNextItem(hWnd, -1, LVNI_SELECTED);
}

bool DeleteListViewItem(HWND hWnd, int selectedIndex) {
    return ListView_DeleteItem(hWnd, selectedIndex);
}

void GetListViewSelectedText(HWND hWnd, int itemIndex, int columnIndex, char* buffer, DWORD bufferSize) {
    wchar_t* wText = new wchar_t[512];

    ListView_GetItemText(hWnd, itemIndex, columnIndex, (LPTSTR)wText, bufferSize);

    size_t convertedChars = 0;
    wcstombs_s(&convertedChars, buffer, bufferSize, wText, _TRUNCATE);

    delete[] wText;
}

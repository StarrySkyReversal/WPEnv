#pragma once

#include "Resource.h"

void CreateListViewControls(HWND hWnd, HINSTANCE hInstance);
void AdjustListViewColumns(HWND hWndListView);

int GetListViewCount(HWND hWnd);
int GetListViewItemInsertIndex(HWND hWnd);

void AddListViewItem(HWND hWnd, int itemIndex, int ColumnIndex, wchar_t* text);
int GetListViewSelectedIndex(HWND hWnd);
bool DeleteListViewItem(HWND hWnd, int selectedIndex);
void GetListViewSelectedText(HWND hWnd, int itemIndex, int columnIndex, wchar_t* buffer, DWORD bufferSize);

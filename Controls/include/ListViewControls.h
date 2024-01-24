#pragma once

#include "Resource.h"

void CreateListViewControls(HWND hWnd, HINSTANCE hInstance);
void AdjustListViewColumns(HWND hWndListView);

int GetListViewCount(HWND hWnd);
int GetListViewItemInsertIndex(HWND hWnd);

void AddListViewItem(HWND hWnd, int itemIndex, int ColumnIndex, const char* text);
int GetListViewSelectedIndex(HWND hWnd);
bool DeleteListViewItem(HWND hWnd, int selectedIndex);
void GetListViewSelectedText(HWND hWnd, int itemIndex, int columnIndex, char* buffer, DWORD bufferSize);

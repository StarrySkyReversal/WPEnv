#pragma once

#include "Resource.h"

extern HWND hWndMain;

void CreateButtonControls(HWND hWnd, HINSTANCE hInstance);

LRESULT CALLBACK CustomButtonSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

#pragma once
#include "Resource.h"

void CreateRichEditControls(HWND hWnd, HINSTANCE hInstance);
void InitializeRichEditLibrary();
void FreeRichEdit();
void ClearRichEdit();
void AppendEditInfo(const wchar_t* string);
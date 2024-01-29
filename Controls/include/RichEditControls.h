#pragma once
#include "Resource.h"

void CreateRichEditControls(HWND hWnd, HINSTANCE hInstance);
void InitializeRichEditLibrary();
void FreeRichEdit();
void ClearRichEdit();
//void AppendEditInfo(const char* string);

void InfoOutput(const char* str);

void WarnOutput(const char* str);

void ErrOutput(const char* str);

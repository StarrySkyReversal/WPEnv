#pragma once

#include "Resource.h"

extern int listBoxWidth;
extern int listBoxHeight;
extern int spacing;

extern int buttonWidth;
extern int buttonHeight;
extern int listBoxPlusSpacing;

extern HFONT hFont;
extern HWND hRichEdit;
extern HWND hProgressBar;
extern HWND hStaticLabel;
extern HWND hListPHP, hListMySQL, hListApache, hListNginx, hListConfig;
extern HWND hButtonDownload, hButtonAddConfig, hButtonStart, hButtonStop, hButtonRestart, hButtonRemove;

typedef struct ControlRect {
    HWND hwnd;
    RECT rect;
} ControlRect;

typedef struct ControlRectArray {
    ControlRect* data;
    size_t count;
    size_t capacity;
} ControlRectArray;

void FreeControlRectArray();
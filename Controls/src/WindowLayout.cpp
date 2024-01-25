#include "framework.h"
#include "WindowLayout.h"

int listBoxWidth = 160;
int listBoxHeight = 160;
int spacing = 10;

int buttonWidth = 60;
int buttonHeight = 30;
int listBoxPlusSpacing = listBoxHeight + spacing;

HFONT hFont;
HWND hRichEdit;
HWND hProgressBar;
HWND hStaticLabel;
HWND hListPHP, hListMySQL, hListApache, hListNginx, hListConfig;
HWND hButtonDownload, hButtonAddConfig, hButtonStart, hButtonStop, hButtonRestart, hButtonRemove;

void WindowAdaptive() {

}
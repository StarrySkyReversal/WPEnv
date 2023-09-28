#include "framework.h"
#include "WindowAdaptive.h"
#include "ListViewControls.h"
#include "WindowLayout.h"

static ControlRectArray* controlRects;
static int initialWindowWidth;
static int initialWindowHeight;

ControlRectArray* CreateControlRectArray() {
    ControlRectArray* arr = (ControlRectArray*)malloc(sizeof(ControlRectArray));
    arr->count = 0;
    arr->capacity = 10;
    arr->data = (ControlRect*)malloc(arr->capacity * sizeof(ControlRect));
    return arr;
}

void AddControlRect(ControlRectArray* arr, HWND hwnd, RECT rect) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        ControlRect* newData = (ControlRect*)realloc(arr->data, arr->capacity * sizeof(ControlRect));
        if (arr->data == NULL) {
            // Handle memory allocation failure
            free(arr->data);
            free(arr);
            return;  // or handle the error appropriately
        }
        arr->data = newData;
    }
    arr->data[arr->count].hwnd = hwnd;
    arr->data[arr->count].rect = rect;
    arr->count++;
}

//void FreeControlRectArray(ControlRectArray* arr) {
//    free(arr->data);
//    free(arr);
//}

void FreeControlRectArray() {
    free(controlRects->data);
    free(controlRects);
}

void AddControlToRects(HWND hWnd, int controlID, ControlRectArray* controlRects) {
    HWND hControl;
    RECT rc;

    hControl = GetDlgItem(hWnd, controlID);
    GetWindowRect(hControl, &rc);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);
    AddControlRect(controlRects, hControl, rc);
}

void InitializeControlsAdaptive(HWND hWnd) {
    controlRects = CreateControlRectArray();

    // For each control, record its initial position and size.
    AddControlToRects(hWnd, IDC_BUTTON_CONFIRM, controlRects);
    AddControlToRects(hWnd, IDC_BUTTON_DOWNLOAD, controlRects);
    AddControlToRects(hWnd, IDC_BUTTON_ADD_CONFIG, controlRects);
    AddControlToRects(hWnd, IDC_BUTTON_REMOVE_CONFIG, controlRects);

    AddControlToRects(hWnd, IDC_BUTTON_START_SERVICE, controlRects);
    AddControlToRects(hWnd, IDC_BUTTON_STOP_SERVICE, controlRects);
    AddControlToRects(hWnd, IDC_BUTTON_RESTART_SERVICE, controlRects);

    AddControlToRects(hWnd, IDC_LISTBOX_PHP, controlRects);
    AddControlToRects(hWnd, IDC_LISTBOX_MYSQL, controlRects);
    AddControlToRects(hWnd, IDC_LISTBOX_APACHE, controlRects);
    AddControlToRects(hWnd, IDC_LISTBOX_NGINX, controlRects);
    AddControlToRects(hWnd, IDC_LISTBOX_CONFIG, controlRects);

    AddControlToRects(hWnd, IDC_PROGRESS, controlRects);
    AddControlToRects(hWnd, IDC_PROGRESS_INFO, controlRects);
    AddControlToRects(hWnd, IDC_RICH_EDIT, controlRects);

    RECT rc;
    GetClientRect(hWnd, &rc);
    initialWindowWidth = rc.right;
    initialWindowHeight = rc.bottom;
}

void SetControlsAdaptive(int width, int height) {
    for (size_t i = 0; i < controlRects->count; i++) {
        HWND hControl = controlRects->data[i].hwnd;
        RECT initialRect = controlRects->data[i].rect;

        int newLeft = initialRect.left * width / initialWindowWidth;
        int newTop = initialRect.top * height / initialWindowHeight;
        int newRight = initialRect.right * width / initialWindowWidth;
        int newBottom = initialRect.bottom * height / initialWindowHeight;

        MoveWindow(hControl, newLeft, newTop, newRight - newLeft, newBottom - newTop, TRUE);
    }

    AdjustListViewColumns(hListConfig);
}
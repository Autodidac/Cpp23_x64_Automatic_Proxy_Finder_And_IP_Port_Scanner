// main.cpp
#include <windows.h>
#include <commctrl.h>
import gui;
import version;

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&ic);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = gui::WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = "InternetScannerWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszMenuName = NULL;  // Enable min/max properly

    RegisterClassA(&wc);

    HWND h = CreateWindowExA(WS_EX_APPWINDOW, wc.lpszClassName, app_version::TITLE,
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 900, 700,
        NULL, NULL, hi, NULL);

    // Center window
    RECT rc;
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &rc, 0);
    int screen_w = rc.right - rc.left, screen_h = rc.bottom - rc.top;
    GetWindowRect(h, &rc);
    int win_w = rc.right - rc.left, win_h = rc.bottom - rc.top;
    int x = (screen_w - win_w) / 2, y = (screen_h - win_h) / 2;
    SetWindowPos(h, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(h, SW_SHOW);
    UpdateWindow(h);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// ==WindhawkMod==
// @id              magnifier-headless
// @name            Magnifier Headless Mode
// @description     Blocks the Magnifier window creation, keeping zoom functionality with win+"-" and win+"+" keyboard shortcuts.
// @version         1.3.0
// @author          BCRTVKCS
// @github          https://github.com/bcrtvkcs
// @twitter         https://x.com/bcrtvkcs
// @homepage        https://grdigital.pro
// @include         magnify.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Magnifier Headless Mode
This mod blocks the Magnifier window from ever appearing, while keeping the zoom functionality (Win + `-` and Win + `+`) available. It also prevents the Magnifier from showing up in the taskbar.

This is achieved by hooking `CreateWindowExW` to detect when the Magnifier's UI window is created. As soon as it's created, it's moved into a hidden "host" window using the `SetParent` API, effectively trapping it and preventing it from ever being seen.
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <windhawk_api.h>

// Global handle to our hidden host window.
HWND g_hHostWnd = NULL;

// Function to check if a window is the Magnifier window by its class name.
BOOL IsMagnifierWindow(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        return FALSE;
    }
    wchar_t className[256] = {0};
    GetClassNameW(hwnd, className, sizeof(className)/sizeof(wchar_t));

    // Check for known Magnifier class names.
    return (wcscmp(className, L"MagUIClass") == 0 ||
            wcscmp(className, L"ScreenMagnifierUIWnd") == 0);
}

// --- HOOKS ---

// CreateWindowExW hook to catch Magnifier window creation.
using CreateWindowExW_t = decltype(&CreateWindowExW);
CreateWindowExW_t CreateWindowExW_Original;
HWND WINAPI CreateWindowExW_Hook(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
    HINSTANCE hInstance, LPVOID lpParam) {

    BOOL isMagnifierClass = FALSE;
    if (((ULONG_PTR)lpClassName & ~(ULONG_PTR)0xffff) != 0) {
        if (wcscmp(lpClassName, L"MagUIClass") == 0 ||
            wcscmp(lpClassName, L"ScreenMagnifierUIWnd") == 0) {
            isMagnifierClass = TRUE;
            // Proactively remove styles that would make the window visible or show it in the taskbar.
            dwStyle &= ~WS_VISIBLE;
            dwExStyle &= ~WS_EX_APPWINDOW;
        }
    }

    HWND hwnd = CreateWindowExW_Original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
                                  nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    // If the created window is the Magnifier UI, immediately re-parent it to our hidden host window.
    if (hwnd && isMagnifierClass) {
        Wh_Log(L"Magnifier Headless: Detected Magnifier window creation (HWND: 0x%p). Re-parenting...", hwnd);
        SetParent(hwnd, g_hHostWnd);
        // Also ensure it's explicitly hidden.
        ShowWindow(hwnd, SW_HIDE);
    }

    return hwnd;
}


// --- MOD INITIALIZATION ---

BOOL Wh_ModInit() {
    Wh_Log(L"Magnifier Headless: Initializing...");

    // 1. Create a hidden window to act as a parent "jail" for the Magnifier UI.
    WNDCLASSW wc = {};
    wc.lpfnWndProc = DefWindowProcW;
    wc.lpszClassName = L"MagnifierHeadlessHost";
    wc.hInstance = GetModuleHandle(NULL);
    RegisterClassW(&wc);

    g_hHostWnd = CreateWindowExW(
        0, wc.lpszClassName, L"Magnifier Headless Host", 0,
        0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL
    );

    if (!g_hHostWnd) {
        Wh_Log(L"Magnifier Headless: Failed to create host window.");
        return FALSE;
    }

    Wh_Log(L"Magnifier Headless: Host window created.");

    // 2. Set up the hook.
    if (!Wh_SetFunctionHook((void*)CreateWindowExW, (void*)CreateWindowExW_Hook, (void**)&CreateWindowExW_Original)) {
        Wh_Log(L"Magnifier Headless: Failed to set up CreateWindowExW hook.");
        DestroyWindow(g_hHostWnd);
        g_hHostWnd = NULL;
        return FALSE;
    }

    Wh_Log(L"Magnifier Headless: All hooks set up successfully.");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Magnifier Headless: Uninitializing...");
    if (g_hHostWnd) {
        DestroyWindow(g_hHostWnd);
        g_hHostWnd = NULL;
    }
    Wh_Log(L"Magnifier Headless: Host window destroyed.");
}

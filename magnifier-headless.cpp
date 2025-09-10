// ==WindhawkMod==
// @id              magnifier-headless
// @name            Magnifier Headless (DIAGNOSTICS)
// @description     Diagnostics build to log window creation and manipulation in magnify.exe.
// @version         2.0.0-diag
// @author          BCRTVKCS
// @github          https://github.com/bcrtvkcs
// @twitter         https://x.com/bcrtvkcs
// @homepage        https://grdigital.pro
// @include         magnify.exe
// ==/WindhawkMod==

#include <windows.h>
#include <windhawk_api.h>
#include <stdio.h>
#include <wchar.h>

// Helper to get window class name for logging
void GetAndLogClassName(HWND hWnd, WCHAR* buffer, size_t bufferSize) {
    if (IsWindow(hWnd)) {
        GetClassNameW(hWnd, buffer, (int)bufferSize);
    } else {
        wcscpy_s(buffer, bufferSize, L"(Invalid Window Handle)");
    }
}

// --- DIAGNOSTIC HOOKS ---
// The following hooks do not modify any behavior. They only log the parameters
// of the functions to help diagnose why the Magnifier window reappears.

using CreateWindowExW_t = decltype(&CreateWindowExW);
CreateWindowExW_t CreateWindowExW_Original;
HWND WINAPI CreateWindowExW_Hook(
    DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu,
    HINSTANCE hInstance, LPVOID lpParam) {

    // Check if lpClassName is a string or an atom
    WCHAR classNameStr[256];
    if ((ULONG_PTR)lpClassName & ~(ULONG_PTR)0xffff) {
        wcscpy_s(classNameStr, sizeof(classNameStr)/sizeof(WCHAR), lpClassName);
    } else {
        swprintf_s(classNameStr, sizeof(classNameStr)/sizeof(WCHAR), L"Atom: %p", lpClassName);
    }

    Wh_Log(L"CreateWindowExW: ClassName=%s, WindowName=%s, Style=0x%lX, ExStyle=0x%lX",
           classNameStr, lpWindowName ? lpWindowName : L"(null)", dwStyle, dwExStyle);

    return CreateWindowExW_Original(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y,
                                  nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

using ShowWindow_t = decltype(&ShowWindow);
ShowWindow_t ShowWindow_Original;
BOOL WINAPI ShowWindow_Hook(HWND hWnd, int nCmdShow) {
    WCHAR className[256] = {0};
    GetAndLogClassName(hWnd, className, 256);
    Wh_Log(L"ShowWindow: hWnd=0x%p (%s), nCmdShow=%d", hWnd, className, nCmdShow);
    return ShowWindow_Original(hWnd, nCmdShow);
}

using SetWindowPos_t = decltype(&SetWindowPos);
SetWindowPos_t SetWindowPos_Original;
BOOL WINAPI SetWindowPos_Hook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
    WCHAR className[256] = {0};
    GetAndLogClassName(hWnd, className, 256);
    Wh_Log(L"SetWindowPos: hWnd=0x%p (%s), uFlags=0x%X", hWnd, className, uFlags);
    return SetWindowPos_Original(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

using SetWindowLongPtrW_t = decltype(&SetWindowLongPtrW);
SetWindowLongPtrW_t SetWindowLongPtrW_Original;
LONG_PTR WINAPI SetWindowLongPtrW_Hook(HWND hWnd, int nIndex, LONG_PTR dwNewLong) {
    WCHAR className[256] = {0};
    GetAndLogClassName(hWnd, className, 256);
    Wh_Log(L"SetWindowLongPtrW: hWnd=0x%p (%s), nIndex=%d, dwNewLong=0x%llX", hWnd, className, nIndex, dwNewLong);
    return SetWindowLongPtrW_Original(hWnd, nIndex, dwNewLong);
}


// --- MOD INITIALIZATION ---

BOOL Wh_ModInit() {
    Wh_Log(L"Magnifier Headless (DIAGNOSTICS) mod loaded.");
    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Magnifier Headless (DIAGNOSTICS) mod unloaded.");
}

BOOL Wh_ModBeforeSymbolLoading() {
    Wh_Log(L"Setting up diagnostic hooks...");
    if (!Wh_SetFunctionHook((void*)CreateWindowExW, (void*)CreateWindowExW_Hook, (void**)&CreateWindowExW_Original) ||
        !Wh_SetFunctionHook((void*)ShowWindow, (void*)ShowWindow_Hook, (void**)&ShowWindow_Original) ||
        !Wh_SetFunctionHook((void*)SetWindowPos, (void*)SetWindowPos_Hook, (void**)&SetWindowPos_Original) ||
        !Wh_SetFunctionHook((void*)SetWindowLongPtrW, (void*)SetWindowLongPtrW_Hook, (void**)&SetWindowLongPtrW_Original)) {
        Wh_Log(L"Failed to set up one or more diagnostic hooks.");
        return FALSE;
    }
    Wh_Log(L"All diagnostic hooks set up successfully.");
    return TRUE;
}

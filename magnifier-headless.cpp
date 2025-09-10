// ==WindhawkMod==
// @id              magnifier-headless
// @name            Magnifier Headless Mode
// @description     Blocks the Magnifier window creation, keeping zoom functionality with win+"-" and win+"+" keyboard shortcuts.
// @version         1.0.1
// @author          BCRTVKCS
// @github          https://github.com/bcrtvkcs
// @twitter         https://x.com/bcrtvkcs
// @homepage        https://grdigital.pro
// @include         magnify.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
Blocks the Magnifier window creation, keeping zoom functionality with win+"-" and win+"+" keyboard shortcuts.
*/
// ==/WindhawkModReadme==

#include <windows.h>
#include <windhawk_api.h>


// Function to check if a window is the Magnifier window by its class name.
BOOL IsMagnifierWindow(HWND hwnd) {
    wchar_t className[256] = {0};
    if (!GetClassNameW(hwnd, className, sizeof(className) / sizeof(wchar_t))) {
        return FALSE;
    }

    // Check for known Magnifier class names. Both "MagUIClass" and "ScreenMagnifierUIWnd"
    // are checked to support different versions of Windows where the class name for
    // the Magnifier window may vary.
    return (wcscmp(className, L"MagUIClass") == 0 ||
            wcscmp(className, L"ScreenMagnifierUIWnd") == 0);
}


// CreateWindowExW hook to aggressively block Magnifier window creation.
using CreateWindowExW_t = decltype(&CreateWindowExW);
CreateWindowExW_t CreateWindowExW_Original;
HWND WINAPI CreateWindowExW_Hook(
    DWORD dwExStyle,
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam
) {
    // Check if the window being created is a Magnifier UI window.
    // We must also check if lpClassName is a string pointer, not an atom.
    if (((ULONG_PTR)lpClassName & ~(ULONG_PTR)0xffff) != 0) {
        if (wcscmp(lpClassName, L"MagUIClass") == 0 ||
            wcscmp(lpClassName, L"ScreenMagnifierUIWnd") == 0) {
            // Aggressively block the window from being created by returning NULL.
            // This is a high-risk strategy, but necessary as other methods have failed.
            // It assumes the zoom functionality does not strictly depend on this window handle.
            SetLastError(ERROR_ACCESS_DENIED);
            return NULL;
        }
    }

    // For all other windows, proceed with the original function.
    return CreateWindowExW_Original(
        dwExStyle, lpClassName, lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam
    );
}

// Mod initialization
BOOL Wh_ModInit() {
    // Hide any Magnifier windows that might already be open when the mod is loaded.
    HWND hwnd = FindWindowW(L"MagUIClass", NULL);
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }

    hwnd = FindWindowW(L"ScreenMagnifierUIWnd", NULL);
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }

    return TRUE;
}

// Mod uninitialization
void Wh_ModUninit() {
    // No action is needed here. Since the window is never created, there is no
    // state to restore when the mod is unloaded. The hook is removed automatically
    // by Windhawk.
}

// Set up hooks before symbol loading.
BOOL Wh_ModBeforeSymbolLoading() {
    // Previous attempts to hide the window with multiple hooks failed. This final,
    // aggressive strategy focuses on the single entry point for window creation,
    // `CreateWindowExW`. By blocking the window here, we prevent it from ever
    // being created, which makes hooks on ShowWindow, SetWindowPos, etc., redundant.
    if (!Wh_SetFunctionHook(
        (void*)GetProcAddress(GetModuleHandleW(L"user32.dll"), "CreateWindowExW"),
        (void*)CreateWindowExW_Hook,
        (void**)&CreateWindowExW_Original)) {
        Wh_Log(L"Failed to hook CreateWindowExW");
        return FALSE;
    }

    return TRUE;
}

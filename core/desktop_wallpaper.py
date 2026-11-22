"""
High-Performance Windows Desktop Wallpaper Injector (C-Level Acceleration)
Embeds a window handle directly between desktop icons (SHELLDLL_DefView)
and the system desktop wallpaper layer (WorkerW) in pure C.
"""

import os
import ctypes
from ctypes import wintypes

NATIVE_DLL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "native_core.dll")

_native_dll = None
if os.path.isfile(NATIVE_DLL_PATH):
    try:
        _native_dll = ctypes.CDLL(NATIVE_DLL_PATH)
    except Exception as e:
        print(f"[DesktopWallpaper] Native DLL load warning: {e}")

user32 = ctypes.windll.user32
WNDENUMPROC = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)


def attach_to_desktop(hwnd: int) -> bool:
    """Pin the specified window handle to the Windows desktop behind icons at C speed."""
    if not hwnd:
        return False

    # 1. Native C execution (Sub-millisecond)
    if _native_dll and hasattr(_native_dll, 'native_attach_to_desktop'):
        try:
            res = _native_dll.native_attach_to_desktop(ctypes.c_void_p(hwnd))
            if res:
                return True
        except Exception as e:
            print(f"[DesktopWallpaper] Native attach fallback: {e}")

    # 2. Python ctypes fallback
    try:
        progman = user32.FindWindowW("Progman", None)
        if not progman:
            return False

        result = wintypes.DWORD()
        user32.SendMessageTimeoutW(progman, 0x052C, 0, 0, 0x0000, 1000, ctypes.byref(result))

        found_workerw = []

        def enum_windows_callback(top_hwnd, _):
            shell_view = user32.FindWindowExW(top_hwnd, None, "SHELLDLL_DefView", None)
            if shell_view:
                workerw = user32.FindWindowExW(None, top_hwnd, "WorkerW", None)
                if workerw:
                    found_workerw.append(workerw)
            return True

        user32.EnumWindows(WNDENUMPROC(enum_windows_callback), 0)
        if found_workerw:
            user32.SetParent(hwnd, found_workerw[0])
            return True
    except Exception as e:
        print(f"[DesktopWallpaper] Fallback attach error: {e}")

    return False

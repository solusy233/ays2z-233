"""
High-Performance Native Windows Transparency & Acrylic Effects (C-Level Acceleration)
Applies DWM blur behind window, Acrylic effect, and frameless transparency directly via native C.
"""

import os
import sys
import ctypes

NATIVE_DLL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "native_core.dll")

_native_dll = None
if os.path.isfile(NATIVE_DLL_PATH):
    try:
        _native_dll = ctypes.CDLL(NATIVE_DLL_PATH)
    except Exception as e:
        print(f"[WindowEffects] Native DLL load warning: {e}")


class MARGINS(ctypes.Structure):
    _fields_ = [
        ('cxLeftWidth', ctypes.c_int),
        ('cxRightWidth', ctypes.c_int),
        ('cyTopHeight', ctypes.c_int),
        ('cyBottomHeight', ctypes.c_int)
    ]


class ACCENT_POLICY(ctypes.Structure):
    _fields_ = [
        ('AccentState', ctypes.c_int),
        ('AccentFlags', ctypes.c_int),
        ('GradientColor', ctypes.c_int),
        ('AnimationId', ctypes.c_int)
    ]


class WINCOMPATTRDATA(ctypes.Structure):
    _fields_ = [
        ('Attribute', ctypes.c_int),
        ('Data', ctypes.c_void_p),
        ('SizeOfData', ctypes.c_size_t)
    ]


def enable_window_transparency(window):
    """Apply native DWM blur / acrylic to the pywebview window on Windows."""
    if sys.platform != 'win32':
        return False

    try:
        native = getattr(window, 'native', None)
        if not native:
            return False

        hwnd = int(native.Handle.ToInt32())

        # 1. Native C execution (Sub-millisecond direct user32/dwmapi call)
        if _native_dll and hasattr(_native_dll, 'native_enable_blur'):
            try:
                res = _native_dll.native_enable_blur(ctypes.c_void_p(hwnd), 4)
                if res:
                    return True
            except Exception as e:
                print(f"[WindowEffects] Native blur fallback: {e}")

        # 2. Python ctypes fallback
        dwmapi = ctypes.windll.dwmapi
        margins = MARGINS(-1, -1, -1, -1)
        dwmapi.DwmExtendFrameIntoClientArea(hwnd, ctypes.byref(margins))

        user32 = ctypes.windll.user32
        if hasattr(user32, 'SetWindowCompositionAttribute'):
            policy = ACCENT_POLICY()
            policy.AccentState = 4  # ACCENT_ENABLE_ACRYLICBLURBEHIND
            policy.AccentFlags = 2
            policy.GradientColor = 0x01FFFFFF
            policy.AnimationId = 0

            data = WINCOMPATTRDATA()
            data.Attribute = 19
            data.Data = ctypes.cast(ctypes.byref(policy), ctypes.c_void_p)
            data.SizeOfData = ctypes.sizeof(policy)
            user32.SetWindowCompositionAttribute(hwnd, ctypes.byref(data))

        return True
    except Exception as e:
        print(f"[WindowEffects] Note: {e}")
        return False

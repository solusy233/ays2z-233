"""
System Operations Module
Handles Windows scheduled shutdown, shutdown cancellation, and Run registry auto-start.
"""

import os
import subprocess
import sys
import winreg

REG_RUN_KEY = r"Software\Microsoft\Windows\CurrentVersion\Run"
APP_REG_NAME = "ays2z-233"

def execute_shutdown(seconds: int = 300) -> bool:
    """Execute Windows timed shutdown."""
    try:
        cmd = f"shutdown /s /t {int(seconds)}"
        subprocess.run(cmd, shell=True, check=True)
        return True
    except Exception as e:
        print(f"Error executing shutdown: {e}")
        return False

def cancel_shutdown() -> bool:
    """Cancel any pending Windows scheduled shutdown."""
    try:
        subprocess.run("shutdown /a", shell=True, check=True)
        return True
    except Exception as e:
        print(f"Error canceling shutdown: {e}")
        return False

def is_autostart_enabled() -> bool:
    """Check if the application is set to run at Windows startup."""
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, REG_RUN_KEY, 0, winreg.KEY_READ) as key:
            value, _ = winreg.QueryValueEx(key, APP_REG_NAME)
            return bool(value)
    except FileNotFoundError:
        return False
    except Exception as e:
        print(f"Error checking autostart: {e}")
        return False

def set_autostart(enabled: bool) -> bool:
    """Enable or disable application auto-start via Windows Registry."""
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, REG_RUN_KEY, 0, winreg.KEY_SET_VALUE) as key:
            if enabled:
                # Target the executable or python main.py
                if getattr(sys, 'frozen', False):
                    app_path = f'"{sys.executable}"'
                else:
                    script_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "main.py"))
                    app_path = f'"{sys.executable}" "{script_path}"'
                winreg.SetValueEx(key, APP_REG_NAME, 0, winreg.REG_SZ, app_path)
            else:
                try:
                    winreg.DeleteValue(key, APP_REG_NAME)
                except FileNotFoundError:
                    pass
        return True
    except Exception as e:
        print(f"Error setting autostart: {e}")
        return False

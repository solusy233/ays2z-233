"""
Core Bridge API Module
Exposed to JavaScript in the Web UI via window.pywebview.api.
Modular, streamlined backend bridge for configuration persistence,
wallpaper categorization & management, weather queries, power-off scheduling,
and native window controls.
"""

import json
import os
import sys
import threading
import urllib.parse
import urllib.request
import webview

from .weather_service import WeatherService
from .msn_weather_crawler import MSNWeatherCrawler
from .wallpaper_manager import WallpaperManagerService
from .system_ops import execute_shutdown, cancel_shutdown, is_autostart_enabled, set_autostart

SETTINGS_FILE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "settings.json")

DEFAULT_SETTINGS = {
    "password": "1",
    "startTime": "23:00",
    "averageTemperature": 21.5,
    "weatherUpdateTime": "2026-09-24T21:04+08:00",
    "locationId": "101180201",
    "cityName": "安阳",
    "wallpaperType": "classic",
    "wallpaperPath": "/assets/images/anime_rainy_night.jpg",
    "wallpaperFile": "",
    "wallpaperAutoSwitch": True,
    "theme": "default",
    "currentTheme": "default",
    "autoStart": False,
    "volume": 0.8
}

class BackendAPI:
    def __init__(self, window_holder):
        self.window_holder = window_holder
        self.settings = self._load_settings()

    def _get_window(self):
        return self.window_holder.get("window")

    def _load_settings(self):
        settings = DEFAULT_SETTINGS.copy()
        if os.path.isfile(SETTINGS_FILE):
            try:
                with open(SETTINGS_FILE, "r", encoding="utf-8") as f:
                    loaded = json.load(f)
                    settings.update(loaded)
            except Exception as e:
                print(f"[API] Error reading settings.json: {e}")
        settings["autoStart"] = is_autostart_enabled()
        return settings

    def _save_settings(self, settings_dict):
        try:
            with open(SETTINGS_FILE, "w", encoding="utf-8") as f:
                json.dump(settings_dict, f, ensure_ascii=False, indent=4)
            self.settings = settings_dict
            return True
        except Exception as e:
            print(f"[API] Error saving settings.json: {e}")
            return False

    # ================= Exposed JS Methods =================

    def get_config(self):
        """Retrieve current configuration."""
        self.settings["autoStart"] = is_autostart_enabled()
        return self.settings

    def save_config(self, new_config):
        """Update and persist configuration."""
        if not isinstance(new_config, dict):
            return {"success": False, "error": "Invalid format"}
        self.settings.update(new_config)
        success = self._save_settings(self.settings)
        return {"success": success, "config": self.settings}

    def verify_password(self, input_password):
        """Validate settings access password."""
        correct_password = str(self.settings.get("password", "1"))
        return str(input_password) == correct_password

    def select_wallpaper_folder(self):
        """Open native folder picker for custom wallpaper directory."""
        win = self._get_window()
        if not win:
            return ""
        result = win.create_file_dialog(webview.FOLDER_DIALOG)
        if result and len(result) > 0:
            return result[0].replace("\\", "/")
        return ""

    def select_wallpaper_file(self):
        """Open native file dialog to pick wallpaper file."""
        win = self._get_window()
        if not win:
            return ""
        file_types = (
            '所有支持壁纸 (*.jpg;*.jpeg;*.png;*.webp;*.mp4;*.webm;*.mkv;*.avi)',
            '图片文件 (*.jpg;*.jpeg;*.png;*.webp;*.bmp;*.gif)',
            '视频文件 (*.mp4;*.webm;*.mkv;*.avi)',
            '所有文件 (*.*)'
        )
        result = win.create_file_dialog(webview.OPEN_DIALOG, allow_multiple=False, file_types=file_types)
        if result and len(result) > 0:
            return result[0].replace("\\", "/")
        return ""

    def get_categorized_wallpapers(self, custom_path=None):
        """
        Get all wallpapers categorized by weather condition
        (sunny, rainy, snow, fog, live, custom).
        """
        paths = [custom_path] if custom_path else None
        return WallpaperManagerService.get_categorized_wallpapers(paths)

    def scan_wallpaper_engine(self, custom_path=None):
        """Scan Steam or custom folder for Wallpaper Engine items."""
        cats = self.get_categorized_wallpapers(custom_path)
        return {"count": len(cats.get("all", [])), "categorized": cats, "wallpapers": cats.get("live", [])}

    def schedule_shutdown(self, seconds=300):
        """Execute Windows timed shutdown."""
        success = execute_shutdown(seconds)
        return {"success": success, "seconds": seconds}

    def cancel_shutdown(self):
        """Cancel Windows scheduled shutdown."""
        success = cancel_shutdown()
        return {"success": success}

    def get_autostart_status(self):
        """Check if autostart is registered."""
        return is_autostart_enabled()

    def set_autostart_status(self, enabled):
        """Set autostart in Windows registry."""
        success = set_autostart(enabled)
        self.settings["autoStart"] = enabled
        self._save_settings(self.settings)
        return {"success": success, "autoStart": enabled}

    def fetch_weather(self, location_id="101180201", **kwargs):
        """Fetch real-time weather data with zero-configuration fallback."""
        loc = location_id or self.settings.get("locationId") or "101180201"
        data = WeatherService.fetch_live_weather(location_id=loc)
        if data:
            return {
                "success": True,
                "data": data,
                "fallback": not data.get("isLive", True)
            }
        return {
            "success": False,
            "error": "获取气象失败，请检查网络连接",
            "fallback": True,
            "data": None
        }

    def fetch_hourly_weather(self, city_name=None):
        """
        Crawl MSN Weather for detailed hourly & multi-day forecast,
        UV, AQI, visibility, pressure, wind, sun and moon data.
        """
        city = city_name or self.settings.get("cityName") or "安阳市"
        if str(city).isdigit():
            city = "安阳市" if str(city) == "101180201" else self.settings.get("cityName", "安阳市")
            
        data = MSNWeatherCrawler.crawl_city_weather(city)
        if data:
            return {"success": True, "data": data}
            
        cached = MSNWeatherCrawler.get_cached(city)
        if cached:
            return {"success": True, "data": cached, "fallback": True}

        fallback = MSNWeatherCrawler.get_fallback_data(city)
        return {"success": True, "data": fallback, "fallback": True}

    def toggle_fullscreen(self):
        """Toggle native window fullscreen mode."""
        win = self._get_window()
        if win:
            try:
                win.toggle_fullscreen()
                is_fs = getattr(win, 'fullscreen', False)
                return {"success": True, "fullscreen": is_fs, "is_fullscreen": is_fs}
            except Exception as e:
                print(f"[API] toggle_fullscreen error: {e}")
        return {"success": False, "fullscreen": False}

    def set_fullscreen(self, fullscreen: bool):
        """Explicitly set native window fullscreen state."""
        win = self._get_window()
        if win:
            try:
                if hasattr(win, 'set_fullscreen'):
                    win.set_fullscreen(bool(fullscreen))
                elif getattr(win, 'fullscreen', False) != bool(fullscreen):
                    win.toggle_fullscreen()
                is_fs = getattr(win, 'fullscreen', bool(fullscreen))
                return {"success": True, "fullscreen": is_fs}
            except Exception as e:
                print(f"[API] set_fullscreen error: {e}")
        return {"success": False, "fullscreen": False}

    def minimize_window(self):
        """Minimize window."""
        win = self._get_window()
        if win:
            win.minimize()

    def hide_to_tray(self):
        """Hide window to system tray."""
        win = self._get_window()
        if win:
            win.hide()

    def quit_application(self):
        """Quit application completely."""
        win = self._get_window()
        if win:
            win.destroy()
        sys.exit(0)

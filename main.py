"""
ays2z-233 Web-Refactored Desktop Assistant
Main Application Entry Point.
Integrates PyWebView (Edge WebView2), Local Streaming HTTP Server,
System Tray, Win32 Process Watchdog, and Wallpaper Engine Importer.
"""

import os
import sys
import webview
from core.server import start_local_server
from core.api import BackendAPI
from core.tray import SystemTrayApp
from core.scheduler import AutoPoweroffScheduler
from core.window_effects import enable_window_transparency

def main():
    # Start internal streaming web server
    server, port = start_local_server(port=0)
    base_url = f"http://127.0.0.1:{port}"
    print(f"[*] ays2z-233 local server running at {base_url}")

    window_holder = {}
    api = BackendAPI(window_holder)
    scheduler = AutoPoweroffScheduler(api)
    scheduler.start()

    # System tray setup
    tray = SystemTrayApp(window_holder)
    tray.start()

    # Create pywebview window with native transparency
    icon_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "assets", "images", "app_icon.ico")
    if not os.path.isfile(icon_path):
        icon_path = None

    window = webview.create_window(
        title="ays2z-233 桌面助手",
        url=f"{base_url}/index.html",
        width=1280,
        height=780,
        min_size=(1060, 680),
        resizable=True,
        shadow=True,
        js_api=api,
        transparent=True
    )
    window_holder["window"] = window

    # Enable native Windows glass / acrylic transparency
    window.events.shown += lambda: enable_window_transparency(window)

    # Handle window close: minimize to tray instead of quitting
    def on_closing():
        window.hide()
        return False  # Prevent closing the window, keeping background scheduler and tray active

    window.events.closing += on_closing

    # Start GUI loop (Edge WebView2)
    webview.start(debug=False)

if __name__ == "__main__":
    main()

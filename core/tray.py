"""
System Tray Integration Module
Provides Windows notification tray icon with quick actions: Show, Hide, Cancel Shutdown, and Exit.
Uses pystray and Pillow.
"""

import os
import threading
from PIL import Image
import pystray
from .system_ops import cancel_shutdown

class SystemTrayApp:
    def __init__(self, window_holder, icon_path=None):
        self.window_holder = window_holder
        self.icon_path = icon_path or os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            "assets", "images", "anys2z.jpg"
        )
        self.tray_icon = None
        self._thread = None

    def _get_window(self):
        return self.window_holder.get("window")

    def show_window(self, icon=None, item=None):
        win = self._get_window()
        if win:
            win.show()
            win.restore()

    def hide_window(self, icon=None, item=None):
        win = self._get_window()
        if win:
            win.hide()

    def do_cancel_shutdown(self, icon=None, item=None):
        cancel_shutdown()
        win = self._get_window()
        if win:
            try:
                win.evaluate_js("window.showToast && window.showToast('已取消待执行关机', 'info');")
            except Exception:
                pass

    def quit_app(self, icon=None, item=None):
        if self.tray_icon:
            self.tray_icon.stop()
        win = self._get_window()
        if win:
            win.destroy()
        os._exit(0)

    def _setup_icon(self):
        if os.path.isfile(self.icon_path):
            try:
                image = Image.open(self.icon_path)
            except Exception:
                image = Image.new('RGB', (64, 64), color=(3, 34, 72))
        else:
            image = Image.new('RGB', (64, 64), color=(3, 34, 72))

        menu = pystray.Menu(
            pystray.MenuItem("显示主窗口", self.show_window, default=True),
            pystray.MenuItem("隐藏到托盘", self.hide_window),
            pystray.MenuItem("取消定时关机", self.do_cancel_shutdown),
            pystray.Menu.SEPARATOR,
            pystray.MenuItem("退出程序", self.quit_app)
        )

        self.tray_icon = pystray.Icon("ays2z-233", image, "ays2z-233 桌面助手", menu)
        self.tray_icon.run()

    def start(self):
        """Run tray icon in a dedicated background thread."""
        self._thread = threading.Thread(target=self._setup_icon, daemon=True)
        self._thread.start()

    def stop(self):
        if self.tray_icon:
            self.tray_icon.stop()

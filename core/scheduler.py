"""
Scheduler Module
Background timer that checks the daily shutdown time, triggers day-of-week music,
notifies weather updates, and schedules the 5-minute pre-shutdown countdown.
"""

from datetime import datetime
import json
import os
import threading
import time
from .system_ops import execute_shutdown, cancel_shutdown

class AutoPoweroffScheduler:
    def __init__(self, api, check_interval=1.0):
        self.api = api
        self.check_interval = check_interval
        self._running = False
        self._thread = None
        self.last_day = -1
        self.started_today = False
        self.shutdown_executed_today = False

    def start(self):
        self._running = True
        self._thread = threading.Thread(target=self._run_loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False

    def _run_loop(self):
        while self._running:
            try:
                self._check_time()
            except Exception as e:
                print(f"Error in scheduler check: {e}")
            time.sleep(self.check_interval)

    def _check_time(self):
        now = datetime.now()
        today_day = now.timetuple().tm_yday

        # Reset daily state on new day
        if today_day != self.last_day:
            self.last_day = today_day
            self.started_today = False
            self.shutdown_executed_today = False

        config = self.api.get_config()
        start_time_str = config.get("startTime", "23:00")
        
        try:
            target_hour, target_min = map(int, start_time_str.split(":"))
        except Exception:
            target_hour, target_min = 23, 0

        # Check if current time is within target minute
        is_target_minute = (now.hour == target_hour and now.minute == target_min)
        
        if is_target_minute and not self.started_today:
            self.started_today = True
            print(f"[Scheduler] Daily scheduled time reached: {start_time_str}")
            
            # 1. Inform Web UI to start music and show weather
            weekday = now.isoweekday() # 1 = Monday ... 7 = Sunday
            win = self.api._get_window()
            if win:
                try:
                    js_code = f"window.onDailyScheduleTriggered && window.onDailyScheduleTriggered({weekday});"
                    win.evaluate_js(js_code)
                except Exception as e:
                    print(f"Error invoking JS daily schedule trigger: {e}")

            # 2. Schedule Windows shutdown in 5 minutes (300 seconds)
            execute_shutdown(300)
            self.shutdown_executed_today = True

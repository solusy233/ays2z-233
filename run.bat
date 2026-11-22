@echo off
cd /d "%~dp0"
title ays2z-233 Desktop Assistant
echo [*] Starting ays2z-233 Web Edition...
python main.py
if errorlevel 1 (
    echo [ERROR] Application exited with error.
    pause
)

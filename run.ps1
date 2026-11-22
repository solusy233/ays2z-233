# ays2z-233 PowerShell Launcher
Set-Location -Path $PSScriptRoot
Write-Host "[*] 正在启动 ays2z-233 桌面助手..." -ForegroundColor Cyan
python main.py
if ($LASTEXITCODE -ne 0) {
    Write-Host "[!] 程序异常退出" -ForegroundColor Red
    Pause
}

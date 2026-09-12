# 引入 Windows API 用于获取和设置系统显示配置
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public class DisplayUtils {
    [DllImport("user32.dll")]
    public static extern int GetSystemMetrics(int nIndex);

    [DllImport("user32.dll")]
    public static extern bool SystemParametersInfo(uint uiAction, uint uiParam, IntPtr pvParam, uint fWinIni);
}
"@

# 获取主显示器的水平分辨率
$screenWidth = [DisplayUtils]::GetSystemMetrics(0)

# 4K 分辨率的标准宽度通常为 3840
if ($screenWidth -ge 3840) {
    Write-Host "检测到 4K 分辨率，正在调整系统缩放到 200%..." -ForegroundColor Green
    
    # 更改注册表中的 DPI 设置（200% 对应 192 DPI）
    Set-ItemProperty -Path "HKCU:\Control Panel\Desktop" -Name "LogPixels" -Value 192 -Type DWord
    Set-ItemProperty -Path "HKCU:\Control Panel\Desktop" -Name "Win8DpiScaling" -Value 1 -Type DWord
    
    Write-Host "设置成功！部分应用程序可能需要注销或重启系统后生效。" -ForegroundColor Yellow
} else {
    Write-Host "当前屏幕宽度为 $screenWidth，未达到 4K (3840) 标准，未作修改。" -ForegroundColor Cyan
}
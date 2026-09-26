# FluentZero CI 截图脚本
# 1) 启动 exe，等窗口
# 2) 主窗口(class=FluentZeroWnd)在 -> PrintWindow 抓 D2D 内容
# 3) 不在 -> app.Create() 失败弹了 MessageBox：读 fz_debug.txt 打印失败步骤，截整屏作证据
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$exe = Join-Path $PWD "bin\x64\Release\FluentZero.exe"
if (-not (Test-Path $exe)) { throw "exe not found: $exe" }

$proc = Start-Process -FilePath $exe -WorkingDirectory $PWD -PassThru
Start-Sleep -Seconds 4

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class FzW {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l, t, r, b; }
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, StringBuilder s, int max);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int max);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
    public delegate bool EnumCB(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumCB cb, IntPtr l);
    public static void Dump() {
        EnumWindows((h, l) => {
            if (IsWindowVisible(h)) {
                var c = new StringBuilder(256); GetClassName(h, c, 256);
                var t = new StringBuilder(256); GetWindowText(h, t, 256);
                Console.WriteLine("WIN [" + c + "] title=" + t);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@

$shotPath = Join-Path $PWD "fluentzero_screenshot.png"
$hwnd = [FzW]::FindWindow("FluentZeroWnd", $null)

if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "!!! 主窗口未出现 —— app.Create() 可能失败，打印诊断:"
    $dbg = Join-Path $PWD "fz_debug.txt"
    if (Test-Path $dbg) {
        Write-Host "===== fz_debug.txt ====="
        Get-Content $dbg -Encoding Unicode | ForEach-Object { Write-Host "  $_" }
        Write-Host "========================"
    } else {
        Write-Host "  (fz_debug.txt 不存在)"
    }
    Write-Host "可见窗口列表:"
    [FzW]::Dump()
    $screen = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap($screen.Width, $screen.Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.X, $screen.Y, 0, 0, $bmp.Size)
    $g.Dispose()
    $bmp.Save($shotPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "已保存整屏截图（含失败 MessageBox）: $shotPath"
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    throw "FluentZero 主窗口未创建（初始化失败）"
}

Write-Host "找到主窗口 handle=$hwnd，用 PrintWindow 抓 D2D 内容"
[FzW]::ShowWindow($hwnd, 9) | Out-Null
[FzW]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Seconds 1

$rect = New-Object FzW+RECT
[FzW]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$w = $rect.r - $rect.l; $h = $rect.b - $rect.t
Write-Host "窗口矩形: ($($rect.l),$($rect.t)) ${w}x${h}"

$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
$ok = [FzW]::PrintWindow($hwnd, $hdc, 2)   # PW_RENDERFULLCONTENT
$g.ReleaseHdc($hdc)
$g.Dispose()
Write-Host "PrintWindow 返回: $ok"

$bmp.Save($shotPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host ("已保存截图: {0} ({1} KB)" -f $shotPath, [math]::Round((Get-Item $shotPath).Length/1KB, 1))

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
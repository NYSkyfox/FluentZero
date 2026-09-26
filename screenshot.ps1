# FluentZero CI 截图脚本：启动 exe -> 找窗口 -> 置前 -> 截窗口区域存 PNG
$ErrorActionPreference = "Stop"

# System.Drawing（pwsh7 需显式加载）
Add-Type -AssemblyName System.Drawing

$exe = Join-Path $PWD "bin\x64\Release\FluentZero.exe"
if (-not (Test-Path $exe)) { throw "exe not found: $exe" }

# 后台启动 GUI 程序（不阻塞）
$proc = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 4   # 等窗口创建 + 首帧渲染

# P/Invoke 声明
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

# 按【类名】查找（第一个参数是类名，第二个是标题）
$hwnd = [FzW]::FindWindow("FluentZeroWnd", $null)
if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "!!! FindWindow by class failed — enumerating visible windows for diagnosis:"
    [FzW]::Dump()
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    throw "FluentZero window (class=FluentZeroWnd) not found"
}
Write-Host "Found window handle: $hwnd"

# 置前 + 激活，等重绘
[FzW]::ShowWindow($hwnd, 9) | Out-Null    # SW_RESTORE
[FzW]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Seconds 2

# 取窗口矩形，截屏
$rect = New-Object FzW+RECT
[FzW]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$w = $rect.r - $rect.l; $h = $rect.b - $rect.t
Write-Host "Window rect: ($($rect.l),$($rect.t)) ${w}x${h}"
if ($w -lt 10 -or $h -lt 10) { throw "bad window size ${w}x${h}" }

$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($rect.l, $rect.t, 0, 0, (New-Object System.Drawing.Size($w, $h)))
$g.Dispose()

$out = Join-Path $PWD "fluentzero_screenshot.png"
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host ("Saved screenshot: {0} ({1} KB)" -f $out, [math]::Round((Get-Item $out).Length/1KB, 1))

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue

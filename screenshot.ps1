# FluentZero CI 截图脚本 v3
# 关键点：
#   - FindWindow 是 user32 的宏（真实导出 FindWindowW/A），P/Invoke 必须 EntryPoint="FindWindowW"
#     本脚本改用 EnumWindows 按类名找窗口（EnumWindows 是真实导出，最可靠）
#   - NOREDIRECTIONBITMAP 窗口内容由 DWM 合成到屏幕，用 CopyFromScreen 截窗口区域
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$exe = Join-Path $PWD "bin\x64\Release\FluentZero.exe"
if (-not (Test-Path $exe)) { throw "exe not found: $exe" }

$proc = Start-Process -FilePath $exe -WorkingDirectory $PWD -PassThru

Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class FzW {
    public static IntPtr found = IntPtr.Zero;
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l, t, r, b; }
    [DllImport("user32.dll", EntryPoint="GetClassNameW", CharSet=CharSet.Unicode)]
    public static extern int GetClassName(IntPtr h, StringBuilder s, int max);
    [DllImport("user32.dll", EntryPoint="GetWindowTextW", CharSet=CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr h, StringBuilder s, int max);
    public delegate bool EnumCB(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumCB cb, IntPtr l);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    public static IntPtr FindByClass(string cls) {
        found = IntPtr.Zero;
        EnumWindows((h, l) => {
            var sb = new StringBuilder(256);
            GetClassName(h, sb, 256);
            if (sb.ToString() == cls) { found = h; return false; }
            return true;
        }, IntPtr.Zero);
        return found;
    }
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

# 轮询等待窗口出现（最多 12 秒）
$hwnd = [IntPtr]::Zero
for ($i = 0; $i -lt 24; $i++) {
    Start-Sleep -Milliseconds 500
    $hwnd = [FzW]::FindByClass("FluentZeroWnd")
    if ($hwnd -ne [IntPtr]::Zero) { break }
}

$shotPath = Join-Path $PWD "fluentzero_screenshot.png"

if ($hwnd -eq [IntPtr]::Zero) {
    Write-Host "!!! 主窗口未出现 —— 打印诊断:"
    foreach ($f in @("fz_crash.txt", "fz_debug.txt")) {
        $p = Join-Path $PWD $f
        if (Test-Path $p) {
            Write-Host "===== $f ====="
            Get-Content $p -Encoding Unicode | ForEach-Object { Write-Host "  $_" }
            Write-Host ("=" * (6 + $f.Length))
        }
    }
    Write-Host "可见窗口:"
    [FzW]::Dump()
    $screen = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $bmp = New-Object System.Drawing.Bitmap($screen.Width, $screen.Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.X, $screen.Y, 0, 0, $bmp.Size)
    $g.Dispose()
    $bmp.Save($shotPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    throw "FluentZero 主窗口未创建"
}

Write-Host "找到窗口 handle=$hwnd"
[FzW]::ShowWindow($hwnd, 9) | Out-Null
[FzW]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 800   # 等重绘稳定

$rect = New-Object FzW+RECT
[FzW]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$w = $rect.r - $rect.l; $h = $rect.b - $rect.t
Write-Host "窗口矩形: ($($rect.l),$($rect.t)) ${w}x${h}"
if ($w -lt 20 -or $h -lt 20) { throw "窗口尺寸异常 ${w}x${h}" }

# CopyFromScreen 截窗口区域（NOREDIRECTIONBITMAP 内容 DWM 已合成到屏幕）
$bmp = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($rect.l, $rect.t, 0, 0, (New-Object System.Drawing.Size($w, $h)),
                 [System.Drawing.CopyPixelOperation]::SrcCopy)
$g.Dispose()
$bmp.Save($shotPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host ("已保存截图: {0} ({1} KB)" -f $shotPath, [math]::Round((Get-Item $shotPath).Length/1KB, 1))

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
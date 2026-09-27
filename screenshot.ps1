# FluentZero CI 截图脚本 v4（全屏截图）
# 关键点：
#   - FindWindow 是 user32 的宏（真实导出 FindWindowW/A），P/Invoke 必须 EntryPoint="FindWindowW"
#     本脚本改用 EnumWindows 按类名找窗口（EnumWindows 是真实导出，最可靠）
#   - NOREDIRECTIONBITMAP 窗口半透明，磨砂(DWM)会把窗口后方内容模糊透出；
#     Runner 上窗口后方是 Actions 终端，故截图会透出终端日志。
#   - 本版本改为【全屏截图】：截取整个虚拟桌面，便于看清窗口在桌面的位置、
#     任务栏，以及磨砂背后实际是什么。
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
    [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool IsZoomed(IntPtr h);
    // 最小化除 keepCls 之外所有顶层可见窗口（保留桌面/任务栏），排除终端等干扰
    public static void MinimizeExcept(string keepCls) {
        int minimized = 0;
        EnumWindows((h, l) => {
            if (!IsWindow(h) || !IsWindowVisible(h)) return true;
            var cb = new StringBuilder(256); GetClassName(h, cb, 256);
            string cls = cb.ToString();
            // 保留目标窗口
            if (cls == keepCls) return true;
            // 保留系统桌面组件（任务栏/桌面/壁纸/UWP 宿主）
            if (cls == "Shell_TrayWnd" || cls == "Progman" || cls == "WorkerW"
                || cls == "Shell_SecondaryTrayWnd" || cls == "ShellDesktop"
                || cls == "Windows.UI.Core.CoreWindow") return true;
            ShowWindow(h, 6); // SW_MINIMIZE = 6
            minimized++;
            return true;
        }, IntPtr.Zero);
        Console.WriteLine("    MinimizeExcept(" + keepCls + "): " + minimized + " 个顶层窗口已最小化");
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
# 1) 最小化除 FluentZero 外的所有顶层窗口（隐藏 Actions 终端等干扰源）
[FzW]::MinimizeExcept("FluentZeroWnd")
Start-Sleep -Milliseconds 600
# 2) 最大化 FluentZero（铺满屏幕：内容区更大、磨砂背后只剩纯净桌面，排除终端日志干扰）
[FzW]::ShowWindow($hwnd, 3) | Out-Null       # SW_MAXIMIZE = 3
[FzW]::SetForegroundWindow($hwnd) | Out-Null
Start-Sleep -Milliseconds 1200                # 等最大化重绘 + DWM 稳定

$rect = New-Object FzW+RECT
[FzW]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
$w = $rect.r - $rect.l; $h = $rect.b - $rect.t
Write-Host "窗口矩形: ($($rect.l),$($rect.t)) ${w}x${h}"
if ($w -lt 20 -or $h -lt 20) { throw "窗口尺寸异常 ${w}x${h}" }

# 全屏截图：截取整个虚拟桌面（含窗口、任务栏、磨砂背后内容）
$screen = [System.Windows.Forms.SystemInformation]::VirtualScreen
$sw = $screen.Width; $sh = $screen.Height
Write-Host "虚拟屏幕: ($($screen.X),$($screen.Y)) ${sw}x${sh}"
if ($sw -lt 20 -or $sh -lt 20) {
    # 兜底：虚拟屏幕取不到时退回窗口区域截图
    $sw = $w; $sh = $h
    $bmp = New-Object System.Drawing.Bitmap($sw, $sh)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($rect.l, $rect.t, 0, 0, (New-Object System.Drawing.Size($sw, $sh)))
    $g.Dispose()
} else {
    $bmp = New-Object System.Drawing.Bitmap($sw, $sh)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.X, $screen.Y, 0, 0, (New-Object System.Drawing.Size($sw, $sh)))
    $g.Dispose()
}
$bmp.Save($shotPath, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host ("已保存全屏截图: {0} ({1} KB)" -f $shotPath, [math]::Round((Get-Item $shotPath).Length/1KB, 1))

Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
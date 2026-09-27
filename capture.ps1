# capture.ps1 — 桌面导演：按场景摆好桌面 → 全屏截图
# 用法: pwsh -File capture.ps1 -Scenario raw_desktop
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('raw_desktop','settings_about','settings_personalization','fluentzero','fluentzero_over_settings')]
    [string]$Scenario,
    # 目标分辨率，逗号分隔 WxH，可多个按顺序尝试（如 "1920x1080,1600x900"）
    [string]$Resolution = "1920x1080,1600x900,1280x1024"
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms

$shotPath = Join-Path $PWD "capture_screenshot.png"
Write-Host "=== 场景: $Scenario ==="

# ---------- P/Invoke ----------
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class Cap {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int l, t, r, b; }
    [DllImport("user32.dll", EntryPoint="GetClassNameW", CharSet=CharSet.Unicode)]
    public static extern int GetClassName(IntPtr h, StringBuilder s, int max);
    [DllImport("user32.dll", EntryPoint="GetWindowTextW", CharSet=CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr h, StringBuilder s, int max);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    public delegate bool EnumCB(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumCB cb, IntPtr l);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, IntPtr ei);
    public static bool Found = false;
    public static IntPtr Hwnd = IntPtr.Zero;
    // 按类名找窗口
    public static void FindByClass(string cls) {
        Found = false;
        EnumWindows((h, l) => {
            var sb = new StringBuilder(256); GetClassName(h, sb, 256);
            if (sb.ToString() == cls) { Found = true; Hwnd = h; return false; }
            return true;
        }, IntPtr.Zero);
    }
    // 按标题包含关键词 + 可见 找窗口（Settings 等窗口标题随语言/版本变化，用 Contains 最稳）
    public static void FindByTitleContains(string kw) {
        Found = false;
        EnumWindows((h, l) => {
            if (!IsWindowVisible(h)) return true;
            var sb = new StringBuilder(512); GetWindowText(h, sb, 512);
            if (sb.Length > 0 && sb.ToString().IndexOf(kw, StringComparison.OrdinalIgnoreCase) >= 0) {
                Found = true; Hwnd = h; return false;
            }
            return true;
        }, IntPtr.Zero);
    }
    // 最小化所有顶层可见窗口（保留桌面/任务栏），露出原始桌面
    // 比 keybd_event 发 Win+D 可靠：Runner 会话里 Win 键属系统保留键常被拦截，
    // 直接 ShowWindow(SW_MINIMIZE) 每个顶层窗口能确保 agent 终端窗口被隐藏
    [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr h);
    public static void MinimizeAll() {
        int minimized = 0;
        EnumWindows((h, l) => {
            if (!IsWindow(h) || !IsWindowVisible(h)) return true;
            var cb = new StringBuilder(256); GetClassName(h, cb, 256);
            string cls = cb.ToString();
            // 保留系统桌面组件（任务栏/桌面图标层/壁纸层/UWP 宿主窗口）
            if (cls == "Shell_TrayWnd" || cls == "Progman" || cls == "WorkerW"
                || cls == "Shell_SecondaryTrayWnd" || cls == "ShellDesktop"
                || cls == "Windows.UI.Core.CoreWindow") return true;
            ShowWindow(h, 6); // SW_MINIMIZE = 6
            minimized++;
            return true;
        }, IntPtr.Zero);
        Console.WriteLine("    MinimizeAll: " + minimized + " 个顶层窗口已最小化");
    }
    // ---- 屏幕分辨率 ----
    [StructLayout(LayoutKind.Sequential)]
    public struct DEVMODE {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string dmDeviceName;
        public short dmSpecVersion;
        public short dmDriverVersion;
        public short dmSize;
        public short dmDriverExtra;
        public int dmFields;
        public int dmPositionX;
        public int dmPositionY;
        public int dmDisplayOrientation;
        public int dmDisplayFixedOutput;
        public int dmColor;
        public int dmDuplex;
        public int dmYResolution;
        public int dmTTOption;
        public int dmCollate;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)] public string dmFormName;
        public int dmLogPixels;
        public int dmBitsPerPel;
        public int dmPelsWidth;
        public int dmPelsHeight;
        public int dmDisplayFlags;
        public int dmDisplayFrequency;
        public int dmICMMethod;
        public int dmICMIntent;
        public int dmMediaType;
        public int dmDitherType;
        public int dmReserved1;
        public int dmReserved2;
        public int dmPanningWidth;
        public int dmPanningHeight;
    }
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    public static extern int EnumDisplaySettings(string name, int modeNum, ref DEVMODE dm);
    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    public static extern int ChangeDisplaySettings(ref DEVMODE dm, int flags);
    // 改分辨率：返回 true 表示成功（DISP_CHANGE_SUCCESSFUL=0 / RESTART=1）
    public static bool SetResolution(int w, int h) {
        var dm = new DEVMODE();
        dm.dmSize = (short)Marshal.SizeOf(typeof(DEVMODE));
        if (EnumDisplaySettings(null, -1, ref dm) != 0) return false;
        dm.dmPelsWidth = w;
        dm.dmPelsHeight = h;
        dm.dmFields = 0x800000 | 0x100000; // DM_PELSWIDTH | DM_PELSHEIGHT
        int r = ChangeDisplaySettings(ref dm, 0);
        return r == 0 || r == 1;
    }
}
"@

# ---------- 工具函数 ----------
function Set-DesktopResolution {
    param([string]$List)
    if ([string]::IsNullOrWhiteSpace($List)) { Write-Host "  跳过分辨率设置（未指定）"; return }
    foreach ($item in ($List -split ',')) {
        $t = $item.Trim()
        if ($t -notmatch '^(\d+)x(\d+)$') { continue }
        $w = [int]$Matches[1]; $h = [int]$Matches[2]
        if ($w -lt 800 -or $h -lt 600) { continue }
        if ([Cap]::SetResolution($w, $h)) {
            Start-Sleep -Milliseconds 1500   # 等 DWM 按新分辨率重排桌面
            Write-Host "  分辨率已设为 ${w}x${h}"
            return
        } else {
            Write-Host "  ${w}x${h} 不被支持，尝试下一个"
        }
    }
    Write-Host "  警告: 所有目标分辨率均不支持，保持当前分辨率"
}

function Show-Desktop {
    # 强制最小化所有顶层可见窗口，露出原始桌面（比 Win+D 键可靠）
    [Cap]::MinimizeAll()
    Start-Sleep -Milliseconds 1200
    Write-Host "  已最小化所有顶层窗口（终端已隐藏）"
}

function Open-Settings {
    param([string]$Uri, [string]$TitleKw)
    Start-Process $Uri
    Write-Host "  已启动: $Uri"
    # 等 Settings 窗口出现（标题含关键词，最多 20s）
    $ok = $false
    for ($i = 0; $i -lt 40; $i++) {
        Start-Sleep -Milliseconds 500
        [Cap]::FindByTitleContains($TitleKw)
        if ([Cap]::Found) { $ok = $true; break }
    }
    if ($ok) {
        [Cap]::SetForegroundWindow([Cap]::Hwnd) | Out-Null
        Start-Sleep -Milliseconds 2000   # 等页面渲染稳定
    } else {
        Write-Host "  警告: 未匹配到标题含 '$TitleKw' 的窗口，等待 5s 后继续"
        Start-Sleep -Seconds 5
    }
    Write-Host "  Settings 就绪"
}

function Run-FluentZero {
    $exe = Join-Path $PWD "bin\x64\Release\FluentZero.exe"
    if (-not (Test-Path $exe)) { throw "FluentZero.exe not found: $exe（请先构建）" }
    Start-Process -FilePath $exe -WorkingDirectory $PWD
    # 等窗口出现（最多 12s）
    $ok = $false
    for ($i = 0; $i -lt 24; $i++) {
        Start-Sleep -Milliseconds 500
        [Cap]::FindByClass("FluentZeroWnd")
        if ([Cap]::Found) { $ok = $true; break }
    }
    if (-not $ok) { throw "FluentZero 窗口未出现" }
    [Cap]::ShowWindow([Cap]::Hwnd, 9) | Out-Null    # SW_RESTORE
    [Cap]::SetForegroundWindow([Cap]::Hwnd) | Out-Null
    Start-Sleep -Milliseconds 1500
    Write-Host "  FluentZero 就绪"
}

function Take-FullScreen {
    param([string]$Path)
    $screen = [System.Windows.Forms.SystemInformation]::VirtualScreen
    $sw = $screen.Width; $sh = $screen.Height
    Write-Host "  虚拟屏幕: ($($screen.X),$($screen.Y)) ${sw}x${sh}"
    $bmp = New-Object System.Drawing.Bitmap($sw, $sh)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($screen.X, $screen.Y, 0, 0, (New-Object System.Drawing.Size($sw, $sh)))
    $g.Dispose()
    $bmp.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    $kb = [math]::Round((Get-Item $Path).Length / 1KB, 1)
    Write-Host "  截图已保存: $Path (${kb} KB)"
}

# ---------- 分辨率（在所有窗口操作前，确保新分辨率先生效） ----------
Set-DesktopResolution $Resolution
Start-Sleep -Milliseconds 500

# ---------- 场景逻辑 ----------
switch ($Scenario) {
    'raw_desktop' {
        Show-Desktop
    }
    'settings_about' {
        Show-Desktop
        Open-Settings "ms-settings:about" "System"
    }
    'settings_personalization' {
        Show-Desktop
        Open-Settings "ms-settings:personalization" "Personalization"
    }
    'fluentzero' {
        Show-Desktop
        Run-FluentZero
    }
    'fluentzero_over_settings' {
        Show-Desktop
        Open-Settings "ms-settings:about" "System"
        Run-FluentZero
    }
}

# ---------- 截图 ----------
Take-FullScreen $shotPath
Write-Host "=== 完成: $Scenario ==="
#pragma once

// Windows 系统设置读取（注册表 / DPI）
// 只依赖 Win32 API，不依赖其他 fz 层
namespace fz {

// 读取系统强调色（HKCU\...\ThemeManager\AccentColor），默认 #0078D7
D2D1_COLOR_F ReadAccent();
// 系统是否偏好浅色主题（AppsUseLightTheme）
bool SystemPrefersLight();
// 运行时检测系统当前深浅主题（DWM 优先，回退注册表）；hwnd 可为 null
// 用于实时跟随：Win10 1903+ 走 DWMWA_USE_LIGHT_THEME，1809 回退色板亮度，再无则注册表
bool SystemUsesLightTheme(HWND hwnd);
// 获取窗口所在监视器的有效 DPI（96 = 100%），GetDpiForMonitor 不可用时返回 96
UINT GetEffectiveDpi(HWND hwnd);

} // namespace fz
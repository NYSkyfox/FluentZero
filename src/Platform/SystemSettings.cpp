#include "pch.h"
#include "SystemSettings.h"

namespace fz {

D2D1_COLOR_F ReadAccent() {
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
            0, KEY_READ, &k) == ERROR_SUCCESS) {
        DWORD v = 0, size = sizeof(v), type = 0;
        if (RegQueryValueExW(k, L"AccentColor", nullptr, &type, (LPBYTE)&v, &size) == ERROR_SUCCESS
            && type == REG_DWORD) {
            // 注册表值为 0x00BBGGRR（小端 DWORD 存 RGB）
            v &= 0x00FFFFFF;
            v = ((v >> 16) & 0xFF) | ((v >> 8) & 0xFF00) | ((v & 0xFF) << 16);
            RegCloseKey(k);
            D2D1_COLOR_F c{};
            c.r = ((v >> 16) & 0xFF) / 255.0f;
            c.g = ((v >> 8) & 0xFF) / 255.0f;
            c.b = (v & 0xFF) / 255.0f;
            c.a = 1.0f;
            return c;
        }
        RegCloseKey(k);
    }
    // 默认 #0078D7
    D2D1_COLOR_F def{};
    def.r = 0; def.g = 0x78 / 255.0f; def.b = 0xD7 / 255.0f; def.a = 1.0f;
    return def;
}

bool SystemPrefersLight() {
    bool light = true;
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0, KEY_READ, &k) == ERROR_SUCCESS) {
        DWORD v = 1, size = sizeof(v), type = 0;
        if (RegQueryValueExW(k, L"AppsUseLightTheme", nullptr, &type, (LPBYTE)&v, &size) == ERROR_SUCCESS)
            light = (v == 1);
        RegCloseKey(k);
    }
    return light;
}

bool SystemUsesLightTheme(HWND hwnd) {
    if (hwnd) {
        // 1) 首选 DWMWA_USE_LIGHT_THEME（=36，Win10 1903+/Win11）：直接返回深浅
        BOOL useLight = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, 36 /*DWMWA_USE_LIGHT_THEME*/,
                (LPVOID)&useLight, sizeof(useLight))))
            return useLight != FALSE;
        // 2) 回退 DWMWA_COLORIZATION_COLOR（=32，Win10 1809+）：按色板亮度判断
        COLORREF ccol = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, 32 /*DWMWA_COLORIZATION_COLOR*/,
                (LPVOID)&ccol, sizeof(ccol)))) {
            int r = (ccol >> 16) & 0xFF, g = (ccol >> 8) & 0xFF, b = ccol & 0xFF;
            float lum = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
            return lum >= 0.5f;
        }
    }
    // 3) 兜底：注册表 AppsUseLightTheme
    return SystemPrefersLight();
}

UINT GetEffectiveDpi(HWND hwnd) {
    // 运行时解析 GetDpiForMonitor（Win8.1+ 均有，避免链接期依赖）
    static BOOL (WINAPI *pGetDpi)(HMONITOR, DWORD, UINT*, UINT*) = [] {
        return (decltype(pGetDpi))
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForMonitor");
    }();
    UINT dpi = 96;
    if (pGetDpi && hwnd) {
        HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        pGetDpi(mon, 0 /*MDT_EFFECTIVE_DPI*/, nullptr, &dpi);
    }
    return dpi;
}

} // namespace fz
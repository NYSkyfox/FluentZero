#include "pch.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

// ============================================================================
//  FluentZero — Windows 10 (2017-2018) 风格 Fluent Design 最小演示
//
//  单文件、零第三方依赖、零附带 DLL。全部使用系统自带组件：
//    win32 / d2d1.dll / dwrite.dll / dcomp.dll / d3d11.dll / dwmapi.dll
//
//  渲染架构（AcrylicMenus 同款，Win10 1607+ / Win11 通用）：
//    HWND（WS_EX_NOREDIRECTIONBITMAP，保留原生标题栏）
//        ├── DwmEnableBlurBehindWindow -> DWM 模糊窗口后方的桌面内容
//        └── DXGI flip swap chain（SetHwnd 直接绑定，预乘 alpha）
//              └── ID2D1DeviceContext1（背面缓冲上绘制）
//                   半透明像素 -> DWM 合成时把模糊透出 = Acrylic
//
//  演示的 Win10 Fluent 要素：
//    1. Acrylic 模糊背景 + 主题底色（浅 #F4F4F4 / 深 #202020）
//    2. 系统强调色：注册表 HKCU\...\ThemeManager\AccentColor（默认 #0078D7）
//    3. 深浅主题自动跟随：AppsUseLightTheme
//    4. Button：hover 提亮 / pressed 压暗 / Reveal 描边
//       （150ms ease-out cubic，对齐 WinUI Button 的 Reveal 交互）
//    5. Segoe MDL2 Assets 图标字体（Home / Settings / Refresh / Add）
//
//  产物：静态链接 CRT 的单 exe，Win10 1809+ / Win11。
// ============================================================================

using namespace Microsoft::WRL;

namespace fz {

// ------------------------------ 工具函数 ----------------------------------
static inline float FzMx(float a, float b) { return a > b ? a : b; }
static inline float Fzmn(float a, float b) { return a < b ? a : b; }
static inline float Clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }
static inline float EaseOut(float t) { float u = 1 - Clamp01(t); return 1 - u * u * u; }

static inline D2D1_COLOR_F Premul(D2D1_COLOR_F c) {
    D2D1_COLOR_F o = c;
    o.r *= c.a; o.g *= c.a; o.b *= c.a;
    return o;
}

static inline D2D1_COLOR_F Brighten(D2D1_COLOR_F c, float amt) {
    D2D1_COLOR_F o = c;
    o.r = Fzmn(1.0f, o.r * (1 + amt));
    o.g = Fzmn(1.0f, o.g * (1 + amt));
    o.b = Fzmn(1.0f, o.b * (1 + amt));
    return o;
}

// 圆角矩形（C 风格结构体，跨 SDK 版本稳定）
static inline D2D1_ROUNDED_RECT_F FzRR(float l, float t, float r, float b, float rad) {
    D2D1_ROUNDED_RECT_F rc{};
    rc.rect.left = l; rc.rect.top = t; rc.rect.right = r; rc.rect.bottom = b;
    rc.radiusX = rad; rc.radiusY = rad;
    return rc;
}

// 颜色构造辅助
static inline D2D1_COLOR_F FzCol(float r, float g, float b, float a = 1.0f) {
    D2D1_COLOR_F c{};
    c.r = r; c.g = g; c.b = b; c.a = a;
    return c;
}

static std::wstring HexOf(D2D1_COLOR_F c) {
    char buf[8];
    sprintf_s(buf, "#%02X%02X%02X",
              (int)(c.r * 255 + 0.5f), (int)(c.g * 255 + 0.5f), (int)(c.b * 255 + 0.5f));
    return buf;
}

// ------------------------------ 系统设置读取 -------------------------------
// 强调色：HKCU\Software\Microsoft\Windows\CurrentVersion\ThemeManager\AccentColor (REG_DWORD)
static D2D1_COLOR_F ReadAccent() {
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
    D2D1_COLOR_F def{};
    def.r = 0; def.g = 0x78 / 255.0f; def.b = 0xD7 / 255.0f; def.a = 1.0f;
    return def; // 默认 #0078D7
}

static bool SystemPrefersLight() {
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

// ------------------------------ Fluent 主题 --------------------------------
struct FluentTheme {
    bool light = true;
    D2D1_COLOR_F bg;            // Acrylic 底色（带 alpha，模糊从透明处透出）
    D2D1_COLOR_F card;
    D2D1_COLOR_F cardBorder;
    D2D1_COLOR_F text1;         // 主文字
    D2D1_COLOR_F text2;         // 次级文字
    D2D1_COLOR_F textOnAccent;
    D2D1_COLOR_F accent;
    D2D1_COLOR_F btnFill;
    D2D1_COLOR_F btnBorder;
    D2D1_COLOR_F btnText;
    D2D1_COLOR_F reveal;        // Reveal 描边颜色

    static FluentTheme Create() {
        FluentTheme t;
        t.light = SystemPrefersLight();
        t.accent = ReadAccent();
        if (t.light) {
            t.bg           = FzCol(0.957f, 0.957f, 0.957f, 0.55f);  // #F4F4F4 @55%
            t.card         = FzCol(1, 1, 1, 0.90f);
            t.cardBorder   = FzCol(0, 0, 0, 0.08f);
            t.text1        = FzCol(0, 0, 0, 0.96f);
            t.text2        = FzCol(0, 0, 0, 0.55f);
            t.textOnAccent = FzCol(1, 1, 1, 1);
            t.btnFill      = FzCol(0.98f, 0.98f, 0.98f, 0.95f);
            t.btnBorder    = FzCol(0, 0, 0, 0.10f);
            t.btnText      = FzCol(0, 0, 0, 0.96f);
            t.reveal       = FzCol(0, 0, 0, 0.30f);
        } else {
            t.bg           = FzCol(0.125f, 0.125f, 0.125f, 0.60f); // #202020
            t.card         = FzCol(0.17f, 0.17f, 0.17f, 0.90f);
            t.cardBorder   = FzCol(1, 1, 1, 0.08f);
            t.text1        = FzCol(1, 1, 1, 0.96f);
            t.text2        = FzCol(1, 1, 1, 0.55f);
            t.textOnAccent = FzCol(1, 1, 1, 1);
            t.btnFill      = FzCol(0.20f, 0.20f, 0.20f, 0.95f);
            t.btnBorder    = FzCol(1, 1, 1, 0.10f);
            t.btnText      = FzCol(1, 1, 1, 0.96f);
            t.reveal       = FzCol(1, 1, 1, 0.30f);
        }
        return t;
    }
};

// ------------------------------ Reveal 按钮 --------------------------------
struct Button {
    std::wstring text;
    UINT32 glyph = 0;      // Segoe MDL2 Assets 码位，0 = 无图标
    bool primary = false;
    float x = 0, y = 0, w = 0, h = 0;   // 逻辑坐标（96dpi 基准）
    float hoverT = 0, pressT = 0, revealT = 0;  // 动画进度 0..1
    bool hot = false, pressed = false;
};

// ------------------------------ 渲染后端 -----------------------------------
struct Renderer {
    ComPtr<ID3D11Device> d3d;
    ComPtr<ID3D11DeviceContext> d3dCtx;
    ComPtr<IDXGISwapChain1> swap;
    ComPtr<ID2D1Device> d2dDevice;
    ComPtr<ID2D1DeviceContext1> dc;
    ComPtr<ID2D1Bitmap1> backBuf;
    ComPtr<IDWriteFactory> dw;
    bool ok = false;        // Acrylic 是否生效（失败也能跑，只是无模糊）
    bool drawing = false;

    HRESULT Init(HWND hwnd, int w, int h) {
        // 1) 窗口不重定向位图：swap chain 内容直接交给 DWM 合成
        LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if (!(ex & WS_EX_NOREDIRECTIONBITMAP)) {
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_NOREDIRECTIONBITMAP);
            SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        // 2) Acrylic：DWM 模糊（Win10 1607+）
        DWM_BLURBEHIND bb{};
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = TRUE;
        if (SUCCEEDED(DwmEnableBlurBehindWindow(hwnd, &bb))) ok = true;

        // 3) D3D11
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                &d3d, nullptr, &d3dCtx)))
            return E_FAIL;

        // 4) DXGI factory
        ComPtr<IDXGIDevice> dxgiDev;
        if (FAILED(d3d.As(&dxgiDev)) || !dxgiDev) return E_FAIL;
        ComPtr<IDXGIFactory2> factory;
        if (FAILED(dxgiDev->GetParent(IID_PPV_ARGS(&factory))) || !factory) return E_FAIL;

        // 5) flip swap chain（composition 创建，随后 SetHwnd 绑窗口）
        DXGI_SWAP_CHAIN_DESC1 sd{};
        sd.Width = FzMx(1, w); sd.Height = FzMx(1, h);
        sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 2;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        if (FAILED(factory->CreateSwapChainForComposition(&sd, nullptr, 0, &swap)))
            return E_FAIL;
        RECT rc; GetClientRect(hwnd, &rc);
        if (FAILED(swap->SetHwnd(hwnd, &rc))) return E_FAIL;

        // 6) D2D1 device + DeviceContext1（需要 1.1 接口才能用 DXGI 资源做位图）
        if (FAILED(D2D1CreateDevice(d3d.Get(), nullptr, &d2dDevice))) return E_FAIL;
        if (FAILED(D2D1CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc)))
            return E_FAIL;
        if (FAILED(UpdateBackBuffer())) return E_FAIL;

        // 7) DirectWrite
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory), (IUnknown**)dw.GetAddressOf())))
            return E_FAIL;
        return S_OK;
    }

    HRESULT UpdateBackBuffer() {
        ComPtr<IDXGIResource> res;
        if (FAILED(swap->GetBuffer(0, IID_PPV_ARGS(&res))) || !res) return E_FAIL;
        ComPtr<ID3D11Texture2D> tex;
        if (FAILED(res.As(&tex))) return E_FAIL;
        backBuf.Reset();
        return dc->CreateBitmapFromDxgiResource(tex.Get(), nullptr, &backBuf);
    }

    HRESULT Resize(HWND hwnd, int w, int h) {
        if (!swap || w < 1 || h < 1) return E_FAIL;
        if (FAILED(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0))) return E_FAIL;
        RECT rc; GetClientRect(hwnd, &rc);
        if (FAILED(swap->SetHwnd(hwnd, &rc))) return E_FAIL;
        return UpdateBackBuffer();
    }

    void BeginDraw() {
        drawing = SUCCEEDED(dc->BeginDraw());
        if (drawing) dc->SetTarget(backBuf.Get());
    }


#include "pch.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dcomp.lib")
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
//  渲染架构：
//    HWND（普通带标题栏窗口）
//        ├── DwmEnableBlurBehindWindow -> Acrylic 模糊（Win10 1607+ 经典实现）
//        └── IDCompositionTarget（关联 HWND）
//              └── DCompSurface
//                    └── D2D1DeviceContext（DXGI flip swap chain 背面缓冲）
//                         半透明像素 -> DWM 把窗口后方的模糊透出 = Acrylic
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
using namespace D2D1;

namespace fz {

// ------------------------------ 缓动函数 ----------------------------------
static inline float Clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }
static inline float EaseOut(float t) { float u = 1 - Clamp01(t); return 1 - u * u * u; }

static inline Color4 Premul(Color4 c) {
    return Color4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
}

static inline Color4 Brighten(Color4 c, float amt) {
    return Color4(min(1.0f, c.r * (1 + amt)),
                  min(1.0f, c.g * (1 + amt)),
                  min(1.0f, c.b * (1 + amt)), c.a);
}

static std::wstring HexOf(Color4 c) {
    char buf[8];
    sprintf_s(buf, "#%02X%02X%02X",
              (int)(c.r * 255 + 0.5f), (int)(c.g * 255 + 0.5f), (int)(c.b * 255 + 0.5f));
    return buf;
}

// ------------------------------ 系统设置读取 -------------------------------
// 强调色：HKCU\Software\Microsoft\Windows\CurrentVersion\ThemeManager\AccentColor (REG_DWORD)
static Color4 ReadAccent() {
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
            return Color4(((v >> 16) & 0xFF) / 255.0f,
                          ((v >> 8) & 0xFF) / 255.0f,
                          (v & 0xFF) / 255.0f, 1.0f);
        }
        RegCloseKey(k);
    }
    return Color4(0 / 255.0f, 0x78 / 255.0f, 0xD7 / 255.0f, 1.0f); // 默认 #0078D7
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
    Color4 bg;            // Acrylic 底色（带 alpha，模糊从透明处透出）
    Color4 card;
    Color4 cardBorder;
    Color4 text1;         // 主文字
    Color4 text2;         // 次级文字
    Color4 textOnAccent;
    Color4 accent;
    Color4 btnFill;
    Color4 btnBorder;
    Color4 btnText;
    Color4 reveal;        // Reveal 描边颜色

    static FluentTheme Create() {
        FluentTheme t;
        t.light = SystemPrefersLight();
        t.accent = ReadAccent();
        if (t.light) {
            t.bg           = Color4(0.957f, 0.957f, 0.957f, 0.55f);  // #F4F4F4 @55%
            t.card         = Color4(1, 1, 1, 0.90f);
            t.cardBorder   = Color4(0, 0, 0, 0.08f);
            t.text1        = Color4(0, 0, 0, 0.96f);
            t.text2        = Color4(0, 0, 0, 0.55f);
            t.textOnAccent = Color4(1, 1, 1, 1);
            t.btnFill      = Color4(0.98f, 0.98f, 0.98f, 0.95f);
            t.btnBorder    = Color4(0, 0, 0, 0.10f);
            t.btnText      = Color4(0, 0, 0, 0.96f);
            t.reveal       = Color4(0, 0, 0, 0.30f);
        } else {
            t.bg           = Color4(0.125f, 0.125f, 0.125f, 0.60f); // #202020
            t.card         = Color4(0.17f, 0.17f, 0.17f, 0.90f);
            t.cardBorder   = Color4(1, 1, 1, 0.08f);
            t.text1        = Color4(1, 1, 1, 0.96f);
            t.text2        = Color4(1, 1, 1, 0.55f);
            t.textOnAccent = Color4(1, 1, 1, 1);
            t.btnFill      = Color4(0.20f, 0.20f, 0.20f, 0.95f);
            t.btnBorder    = Color4(1, 1, 1, 0.10f);
            t.btnText      = Color4(1, 1, 1, 0.96f);
            t.reveal       = Color4(1, 1, 1, 0.30f);
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
    ComPtr<IDCompositionDevice> compDevice;
    ComPtr<IDCompositionTarget> compTarget;
    ComPtr<IDCompositionSurface> compSurface;
    ComPtr<ID3D11Device> d3d;
    ComPtr<ID3D11DeviceContext> d3dCtx;
    ComPtr<IDXGISwapChain1> swap;
    ComPtr<ID2D1Device> d2dDevice;
    ComPtr<ID2D1DeviceContext> dc;
    ComPtr<ID2D1Bitmap1> backBuf;
    ComPtr<IDWriteFactory> dw;
    bool ok = false;

    HRESULT Init(HWND hwnd, int w, int h) {
        // 1) Acrylic：Win10 1607+ BlurBehind（窗口已可见）
        DWM_BLURBEHIND bb{};
        bb.dwFlags = DWM_BB_ENABLE;
        bb.fEnable = TRUE;
        if (SUCCEEDED(DwmEnableBlurBehindWindow(hwnd, &bb))) ok = true;

        // 2) D3D11
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                &d3d, nullptr, &d3dCtx)))
            return E_FAIL;

        // 3) DXGI factory（不指定 HWND，swap chain 交给 DComp 合成）
        ComPtr<IDXGIDevice> dxgiDev;
        if (FAILED(d3d.As(&dxgiDev)) || !dxgiDev) return E_FAIL;
        ComPtr<IDXGIFactory2> factory;
        if (FAILED(dxgiDev->GetParent(IID_PPV_ARGS(&factory))) || !factory) return E_FAIL;

        // 4) DirectComposition 设备 + 与 HWND 关联的 target
        if (FAILED(DCompositionCreateDevice(&compDevice))) return E_FAIL;
        if (FAILED(compDevice->CreateTargetForHwnd(hwnd, FALSE, &compTarget))) return E_FAIL;

        // 5) flip swap chain（半透明格式，配合 Acrylic）
        DXGI_SWAP_CHAIN_DESC1 sd{};
        sd.Width = max(1, w); sd.Height = max(1, h);
        sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 2;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
        if (FAILED(factory->CreateSwapChainForComposition(&sd, nullptr,
                DXGI_SWAP_CHAIN_FLAGS_NONE, &swap)))
            return E_FAIL;

        // 6) DCompSurface 包住 swap chain，挂到 target
        ComPtr<IDXGISwapChain> swap0;
        if (FAILED(swap.As(&swap0))) return E_FAIL;
        if (FAILED(compDevice->CreateSurface(swap0.Get(), nullptr, &compSurface))) return E_FAIL;
        if (FAILED(compTarget->SetRoot(compSurface.Get()))) return E_FAIL;

        // 7) D2D1 device + DC，绑定 swap chain 背面缓冲
        if (FAILED(D2D1CreateDevice(d3d.Get(), &d2dDevice))) return E_FAIL;
        if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &dc)))
            return E_FAIL;
        if (FAILED(UpdateBackBuffer())) return E_FAIL;

        // 8) DirectWrite
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory), IID_PPV_ARGS(&dw))))
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

    HRESULT Resize(int w, int h) {
        if (!swap || w < 1 || h < 1) return E_FAIL;
        if (FAILED(swap->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0))) return E_FAIL;
        return UpdateBackBuffer();
    }

    void BeginDraw() {
        dc->BeginDraw();
        if (dc->GetCurrentBackground() != backBuf.Get())
            dc->SetTarget(backBuf.Get());
    }

    HRESULT EndDraw() {
        if (FAILED(dc->EndDraw())) {
            // GPU 重置之类：重建 swap chain
            swap.Reset();
            return E_FAIL;
        }
        return swap->Present(0, 0);
    }
};

// ------------------------------ 应用本体 -----------------------------------
class App {
public:
    HWND hwnd = nullptr;
    HBRUSH brush = nullptr;
    Renderer r;
    FluentTheme th;
    std::vector<Button> buttons;
    float lastT = 0;
    bool needsDraw = true;
    bool quit = false;
    int primaryClicks = 0;
    std::wstring detailAccent, detailTheme, detailClicks;
    float px = 0, py = 0;
    float titleY = 0, subY = 0, cardX = 0, cardY = 0, cardW = 0, cardH = 0;
    float dpiScale = 1.0f;

    // GetDpiForMonitor 运行时解析（保持链接器不要求新 SDK 导出）
    static BOOL (WINAPI *pGetDpi)(HMONITOR, DWORD, UINT*, UINT*);

    HRESULT Create() {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &App::WndProcStatic;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"FluentZeroWnd";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = nullptr;   // DComp 接管，GDI 不画
        if (!RegisterClassExW(&wc)) return E_FAIL;

        th = FluentTheme::Create();

        hwnd = CreateWindowExW(0, wc.lpszClassName, L"FluentZero",
            WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
            CW_USEDEFAULT, CW_USEDEFAULT, 720, 520,
            nullptr, nullptr, wc.hInstance, this);
        if (!hwnd) return E_FAIL;
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        // DPI
        HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        UINT dpi = 96;
        if (pGetDpi) pGetDpi(mon, 0 /*MDT_EFFECTIVE_DPI*/, nullptr, &dpi);
        dpiScale = dpi / 96.0f;

        RECT rc; GetClientRect(hwnd, &rc);
        r.Init(hwnd, rc.right - rc.left, rc.bottom - rc.top);

        // 按钮（Segoe MDL2 Assets 码位）
        buttons.push_back({ L"Home",     0xE80F, false, 0, 0, 0, 0, 0, 0, 0, false, false });
        buttons.push_back({ L"Settings", 0xE713, false, 0, 0, 0, 0, 0, 0, 0, false, false });
        buttons.push_back({ L"Refresh",  0xE895, false, 0, 0, 0, 0, 0, 0, 0, false, false });
        buttons.push_back({ L"Add item", 0xE710, true,  0, 0, 0, 0, 0, 0, 0, false, false });

        Layout();
        RebuildDetail();
        lastT = (float)GetTickCount64();
        return S_OK;
    }

    // ------------------------------ 布局 ------------------------------
    float Measure(const std::wstring& t, const wchar_t* face, float size, DWRITE_FONT_WEIGHT weight) {
        if (!r.dw) return (float)t.size() * size * 0.6f;
        ComPtr<IDWriteTextFormat> f;
        r.dw->CreateTextFormat(face, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &f);
        if (!f) return (float)t.size() * size * 0.6f;
        ComPtr<IDWriteTextLayout> lay;
        if (FAILED(r.dw->CreateTextLayout(t.c_str(), (UINT32)t.size(), f.Get(), 1e6f, 1e6f, &lay)))
            return (float)t.size() * size * 0.6f;
        D2D1_SIZE_F sz;
        lay->GetMetrics(&sz);
        return sz.width;
    }

    void Layout() {
        RECT rc; GetClientRect(hwnd, &rc);
        float W = rc.right - rc.left, H = rc.bottom - rc.top;
        float m = 28 * dpiScale;
        float y = m;

        titleY = y;          y += 40 * dpiScale;
        subY   = y;          y += 30 * dpiScale;

        // 标准按钮行
        float s = dpiScale, gap = 10 * s, bx = m, by = y;
        for (int i = 0; i < 3; i++) {
            Button& b = buttons[i];
            b.x = bx; b.y = by; b.h = 34 * s;
            float tw = Measure(b.text, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            float iconW = b.glyph ? 18 * s + 6 * s : 0;
            b.w = tw + iconW + 28 * s;
            bx += b.w + gap;
        }
        y += 34 * s + 12 * s;

        // Primary 按钮（强调色）
        Button& pb = buttons[3];
        pb.x = m; pb.y = y; pb.h = 34 * s;
        float ptw = Measure(pb.text, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_SEMIBOLD);
        pb.w = ptw + 18 * s + 6 * s + 32 * s;
        y += 34 * s + 24 * s;

        cardX = m; cardY = y;
        cardW = W - 2 * m;
        cardH = max(110 * s, H - y - m);
        needsDraw = true;
    }

    void RebuildDetail() {
        detailAccent = L"Accent color (system)   " + HexOf(th.accent);
        detailTheme  = th.light ? L"System theme            Light" : L"System theme            Dark";
        detailClicks = L"Primary clicked         " + std::to_wstring(primaryClicks) + L" time(s)";
        needsDraw = true;
    }

    // ------------------------------ 文本辅助 ------------------------------
    ComPtr<ID2D1SolidColorBrush> MakeBrush(Color4 c) {
        ComPtr<ID2D1SolidColorBrush> b;
        r.dc->CreateSolidColorBrush(Premul(c), &b);
        return b;
    }

    void DrawText(const std::wstring& t, float x, float y, float maxW,
                  const wchar_t* face, float size, DWRITE_FONT_WEIGHT weight, Color4 c) {
        if (!r.dw) return;
        ComPtr<IDWriteTextFormat> f;
        if (FAILED(r.dw->CreateTextFormat(face, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &f)))
            return;
        ComPtr<IDWriteTextLayout> lay;
        if (FAILED(r.dw->CreateTextLayout(t.c_str(), (UINT32)t.size(), f.Get(), maxW, 1e6f, &lay)))
            return;
        r.dc->DrawTextLayout(Point2F(x, y), lay.Get(), MakeBrush(c).Get());
    }

    // ------------------------------ 绘制 ------------------------------
    void DrawButton(const Button& b) {
        float s = dpiScale;
        float h = EaseOut(b.hoverT), p = EaseOut(b.pressT), rv = EaseOut(b.revealT);

        Color4 fill = b.primary ? th.accent : th.btnFill;
        fill.a = 1;
        fill = Brighten(fill, 0.04f * h);   // hover 提亮 4%
        fill = Brighten(fill, -0.08f * p);  // pressed 压暗 8%
        Color4 txt = b.primary ? th.textOnAccent : th.btnText;

        float r3 = 3 * s;
        // 填充
        r.dc->FillRoundedRectangle(
            RoundedRect(Point2F(b.x, b.y), Point2F(b.x + b.w, b.y + b.h), r3, r3),
            MakeBrush(fill).Get());
        // 边框（Win10 普通按钮有 1px 描边）
        if (!b.primary) {
            r.dc->DrawRoundedRectangle(
                RoundedRect(Point2F(b.x + 0.5f, b.y + 0.5f),
                            Point2F(b.x + b.w - 0.5f, b.y + b.h - 0.5f), r3, r3),
                MakeBrush(th.btnBorder).Get(), 1);
        }

        // Reveal 描边：整圈描边，alpha 随 revealT 渐入（Win10 Reveal 观感近似）
        if (rv > 0.003f) {
            Color4 rc_ = th.reveal;
            rc_.a = rc_.a * rv;
            r.dc->DrawRoundedRectangle(
                RoundedRect(Point2F(b.x + 0.5f, b.y + 0.5f),
                            Point2F(b.x + b.w - 0.5f, b.y + b.h - 0.5f), r3, r3),
                MakeBrush(rc_).Get(), 1.5f * s);
        }

        // 图标 + 文字（整体水平居中）
        float iconW = b.glyph ? 18 * s : 0, gapW = b.glyph ? 6 * s : 0;
        float tw = Measure(b.text, L"Segoe UI", 13 * s,
            b.primary ? DWRITE_FONT_WEIGHT_SEMIBOLD : DWRITE_FONT_WEIGHT_NORMAL);
        float cx = b.x + (b.w - (iconW + gapW + tw)) * 0.5f;
        float cy = b.y + b.h * 0.5f;
        if (b.glyph) {
            wchar_t g[2] = { (wchar_t)b.glyph, 0 };
            DrawText(g, cx, cy - 9 * s, 40 * s, L"Segoe MDL2 Assets", 15 * s,
                     DWRITE_FONT_WEIGHT_NORMAL, txt);
            cx += iconW + gapW;
        }
        DrawText(b.text, cx, cy - 8 * s, b.w, L"Segoe UI", 13 * s,
            b.primary ? DWRITE_FONT_WEIGHT_SEMIBOLD : DWRITE_FONT_WEIGHT_NORMAL, txt);
    }

    void Draw() {
        RECT rc; GetClientRect(hwnd, &rc);
        float W = rc.right - rc.left, H = rc.bottom - rc.top;
        float s = dpiScale;

        r.BeginDraw();

        // 1) 背景：Acrylic 半透明主题色（DWM 模糊从透明处透出桌面）
        // 注意：D2D 的 Clear 要求预乘 alpha
        r.dc->Clear(Premul(th.bg));

        // 2) 标题 / 副标题
        DrawText(L"FluentZero", 28 * s, titleY, W, L"Segoe UI Semibold", 26 * s,
                 DWRITE_FONT_WEIGHT_SEMIBOLD, th.text1);
        DrawText(L"Windows 10 Fluent Design · 纯 Win32 + Direct2D 手搓 · 零依赖单 exe",
                 28 * s, subY, W, L"Segoe UI", 12 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);

        // 3) 按钮
        for (auto& b : buttons) DrawButton(b);

        // 4) 信息卡
        r.dc->FillRoundedRectangle(
            RoundedRect(Point2F(cardX, cardY), Point2F(cardX + cardW, cardY + cardH), 6 * s, 6 * s),
            MakeBrush(th.card).Get());
        r.dc->DrawRoundedRectangle(
            RoundedRect(Point2F(cardX + 0.5f, cardY + 0.5f),
                        Point2F(cardX + cardW - 0.5f, cardY + cardH - 0.5f), 6 * s, 6 * s),
            MakeBrush(th.cardBorder).Get(), 1);
        // 强调色色块
        r.dc->FillRoundedRectangle(
            RoundedRect(Point2F(cardX + 16 * s, cardY + 16 * s),
                        Point2F(cardX + 16 * s + 36 * s, cardY + 16 * s + 36 * s), 3 * s, 3 * s),
            MakeBrush(Color4(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
        float tx = cardX + 16 * s + 36 * s + 16 * s;
        DrawText(detailAccent, tx, cardY + 18 * s, W, L"Consolas", 13 * s,
                 DWRITE_FONT_WEIGHT_NORMAL, th.text1);
        DrawText(detailTheme,  tx, cardY + 44 * s, W, L"Segoe UI", 13 * s,
                 DWRITE_FONT_WEIGHT_NORMAL, th.text1);
        DrawText(detailClicks, tx, cardY + 70 * s, W, L"Segoe UI", 13 * s,
                 DWRITE_FONT_WEIGHT_NORMAL, th.text1);
        DrawText(L"Reveal hover 150ms ease-out · Segoe MDL2 Assets · Acrylic (BlurBehind)",
                 tx, cardY + 94 * s, W, L"Segoe UI", 11 * s,
                 DWRITE_FONT_WEIGHT_NORMAL, th.text2);

        r.EndDraw();
    }

    // ------------------------------ 输入 ------------------------------
    int HitButton(float x, float y) const {
        for (int i = 0; i < (int)buttons.size(); i++) {
            const Button& b = buttons[i];
            if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) return i;
        }
        return -1;
    }

    void OnMove(float x, float y) {
        px = x; py = y;
        int hit = HitButton(x, y);
        for (int i = 0; i < (int)buttons.size(); i++)
            buttons[i].hot = (i == hit);
        needsDraw = true;
    }

    void OnLButtonDown(float x, float y) {
        int hit = HitButton(x, y);
        if (hit >= 0) {
            buttons[hit].pressed = true;
            SetCapture(hwnd);
            needsDraw = true;
        }
    }

    void OnLButtonUp(float x, float y) {
        if (GetCapture() == hwnd) ReleaseCapture();
        int hit = HitButton(x, y);
        for (int i = 0; i < (int)buttons.size(); i++) {
            Button& b = buttons[i];
            if (b.pressed) {
                b.pressed = false;
                if (i == hit && b.primary) {
                    primaryClicks++;
                    RebuildDetail();
                }
            }
        }
        needsDraw = true;
    }

    // ------------------------------ 动画 ------------------------------
    bool animating = false;

    void Update(float dt) {
        animating = false;
        for (auto& b : buttons) {
            b.hoverT  = Clamp01(b.hoverT + (b.hot ? dt / 0.15f : -dt / 0.15f));
            b.pressT  = Clamp01(b.pressT + (b.pressed ? dt / 0.08f : -dt / 0.12f));
            b.revealT = Clamp01(b.revealT + (b.hot ? dt / 0.15f : -dt / 0.15f));
            if ((b.hoverT > 0 && b.hoverT < 1) || (b.pressT > 0 && b.pressT < 1) ||
                (b.revealT > 0 && b.revealT < 1))
                animating = true;
        }
    }

    // ------------------------------ 消息循环 ------------------------------
    void Run() {
        MSG msg;
        while (!quit) {
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) { quit = true; break; }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            if (quit) break;
            float t = (float)GetTickCount64();
            float dt = min(0.1f, max(0.0f, t - lastT) / 1000.0f);
            lastT = t;
            Update(dt);
            if (needsDraw && r.ok) {
                Draw();
                needsDraw = false;
            }
            // 动画进行中：短睡保证 ~60fps；空闲时阻塞等待消息（零 CPU）
            if (animating || needsDraw) Sleep(16);
            else WaitMessage();
        }
    }

    // ------------------------------ Win32 消息 ------------------------------
    static LRESULT CALLBACK WndProcStatic(HWND h, UINT m, WPARAM w, LPARAM l) {
        App* self = nullptr;
        if (m == WM_NCCREATE) {
            auto* cs = (CREATESTRUCTW*)l;
            self = (App*)cs->lpCreateParams;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
        } else {
            self = (App*)GetWindowLongPtrW(h, GWLP_USERDATA);
        }
        if (self) return self->WndProc(m, w, l);
        return DefWindowProcW(h, m, w, l);
    }

    LRESULT WndProc(UINT m, WPARAM w, LPARAM l) {
        switch (m) {
        case WM_GETMINMAXINFO: {
            auto* mm = (MINMAXINFO*)l;
            mm->ptMinTrackSize.x = 560;
            mm->ptMinTrackSize.y = 420;
            return 0;
        }
        case WM_SIZE:
            if (r.ok && LOWORD(l) > 0 && HIWORD(l) > 0)
                r.Resize(LOWORD(l), HIWORD(l));
            Layout();
            return 0;
        case WM_MOUSEMOVE:
            OnMove(GET_X_LPARAM(l), GET_Y_LPARAM(l));
            {
                // 注册离开检测，让 WM_MOUSELEAVE 真正能收到
                static TRACKMOUSEEVENT tme{};
                tme.cbSize = sizeof(tme);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                tme.dwHoverTime = HOVER_DEFAULT;
                TrackMouseEvent(&tme);
            }
            return 0;
        case WM_MOUSELEAVE:
            for (auto& b : buttons) b.hot = false;
            needsDraw = true;
            return 0;
        case WM_LBUTTONDOWN:
            OnMove(GET_X_LPARAM(l), GET_Y_LPARAM(l));
            OnLButtonDown(GET_X_LPARAM(l), GET_Y_LPARAM(l));
            return 0;
        case WM_LBUTTONUP:
            OnMove(GET_X_LPARAM(l), GET_Y_LPARAM(l));
            OnLButtonUp(GET_X_LPARAM(l), GET_Y_LPARAM(l));
            return 0;
        case WM_ERASEBKGND:
            return 1;  // DComp 接管，抑制 GDI 擦背景
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, m, w, l);
    }
};

BOOL (WINAPI *App::pGetDpi)(HMONITOR, DWORD, UINT*, UINT*) = nullptr;

} // namespace fz

// ------------------------------ 入口 ----------------------------------------
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    // 运行时解析 GetDpiForMonitor（Win8.1+ 均有，避免链接期依赖）
    fz::App::pGetDpi = (decltype(fz::App::pGetDpi))
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForMonitor");

    fz::App app;
    if (FAILED(app.Create())) {
        MessageBoxW(nullptr, L"FluentZero 初始化失败（需要 Win10 1809+）",
                    L"FluentZero", MB_ICONERROR);
        return 1;
    }
    app.Run();
    return 0;
}
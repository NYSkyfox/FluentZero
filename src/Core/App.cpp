#include "pch.h"
#include "Core/App.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"
#include "UI/Button.h"
#include "UI/NavPane.h"
#include "Platform/SystemSettings.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

namespace fz {

// ==================== 创建 ====================

HRESULT App::Create() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &App::WndProcStatic;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"FluentZeroWnd";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    if (!RegisterClassExW(&wc))
        return E_FAIL;

    th = FluentTheme::Create();

    // 按钮必须先于 CreateWindowExW 填充：
    // CreateWindowExW / ShowWindow 会同步发 WM_SIZE → Layout() 访问 buttons[i]，
    // 若此时 buttons 为空则 operator[] 越界 → 野引用写 → 空指针写崩溃(0xC0000005)
    buttons.push_back({ L"Home",     0xE80F, false });
    buttons.push_back({ L"Settings", 0xE713, false });
    buttons.push_back({ L"Refresh",  0xE895, false });
    buttons.push_back({ L"Add item", 0xE710, true  });

    // 导航项（同样必须先于 CreateWindowExW 填充，防 WM_SIZE 早到越界）
    navItems.push_back({ L"Home",     0xE80F, 0, 0, 0, false, true  });
    navItems.push_back({ L"Settings", 0xE713, 0, 0, 0, false, false });
    navItems.push_back({ L"Accounts", 0xE77B, 0, 0, 0, false, false });
    navItems.push_back({ L"Network",  0xE839, 0, 0, 0, false, false });
    pageTitle = L"Home";

    hwnd = CreateWindowExW(0, wc.lpszClassName, L"FluentZero",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 540,
        nullptr, nullptr, wc.hInstance, this);
    if (!hwnd)
        return E_FAIL;

    // DPI
    UINT dpi = GetEffectiveDpi(hwnd);
    dpiScale = dpi / 96.0f;

    RECT rc; GetClientRect(hwnd, &rc);
    if (FAILED(Init(hwnd, rc.right - rc.left, rc.bottom - rc.top)))
        return E_FAIL;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    Layout();
    RebuildDetail();
    currentLight = th.light;
    lastThemePollMs = GetTickCount64();
    lastT = (float)GetTickCount64();
    return S_OK;
}

// ==================== 布局 ====================

void App::Layout() {
    if (buttons.size() < 4 || navItems.empty()) return;   // 防御：未就绪前不布局
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left, H = rc.bottom - rc.top;
    float s = dpiScale;

    // ---- 左侧导航栏 ----
    navGeo.x = 0; navGeo.y = 0;
    navGeo.w = 180 * s;
    navGeo.h = H;
    float navTop = 48 * s;      // 顶部留白（给窗体标题区）
    float navItemH = 36 * s;
    for (int i = 0; i < (int)navItems.size(); i++) {
        navItems[i].y = navTop + i * navItemH;
        navItems[i].h = navItemH;
    }

    // ---- 右侧内容区 ----
    float m = 28 * s;
    contentX = navGeo.w + m;
    float cw = W - contentX - m;   // 内容区可用宽度
    float y = m;

    titleY = y;          y += 40 * s;
    subY   = y;          y += 30 * s;

    // 标准按钮行
    float gap = 10 * s, bx = contentX, by = y;
    for (int i = 0; i < 3; i++) {
        Button& b = buttons[i];
        b.x = bx; b.y = by; b.h = 34 * s;
        float tw = Measure(b.text, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
        float iconW = b.glyph ? 18 * s + 6 * s : 0;
        b.w = Fzmn(cw, tw + iconW + 28 * s);
        bx += b.w + gap;
    }
    y += 34 * s + 12 * s;

    // Primary 按钮（强调色）
    Button& pb = buttons[3];
    pb.x = contentX; pb.y = y; pb.h = 34 * s;
    float ptw = Measure(pb.text, L"Segoe UI", 13 * s, (DWRITE_FONT_WEIGHT)600);
    pb.w = Fzmn(cw, ptw + 18 * s + 6 * s + 32 * s);
    y += 34 * s + 24 * s;

    cardX = contentX; cardY = y;
    cardW = cw;
    cardH = FzMx(110 * s, H - y - m);
    needsDraw = true;
}

void App::RebuildDetail() {
    detailAccent = L"Accent color (system)   " + HexOf(th.accent);
    detailTheme  = th.light ? L"System theme            Light" : L"System theme            Dark";
    detailClicks = L"Primary clicked         " + std::to_wstring(primaryClicks) + L" time(s)";
    needsDraw = true;
}

void App::CheckThemeChange() {
    ULONGLONG now = GetTickCount64();
    if (now - lastThemePollMs < 500) return;   // 500ms 节流，避免每帧查 DWM
    lastThemePollMs = now;
    bool lightNow = SystemUsesLightTheme(hwnd);
    if (lightNow != currentLight) {
        currentLight = lightNow;
        th = FluentTheme::Create(lightNow);     // 纯映射：深浅 + 当前强调色 → 新色板
        RebuildDetail();                        // 刷新 "System theme: Light/Dark" 文案
        needsDraw = true;
    }
}

// ==================== 绘制 ====================

void App::onDraw() {
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left;
    float s = dpiScale;

    // 1) 背景：Acrylic 半透明主题色
    rt->Clear(Premul(th.bg));

    // 2) 左侧导航栏（磨砂面板 + Reveal 交互）
    DrawNavPane(*this, navGeo, navItems, th, s);

    // 3) 右侧内容区：标题 / 副标题
    DrawText(pageTitle, contentX, titleY, W - contentX, L"Segoe UI", 26 * s,
             (DWRITE_FONT_WEIGHT)600, th.text1);
    DrawText(L"Windows 10 Fluent Design · 纯 Win32 + Direct2D 手搓 · 零依赖单 exe",
             contentX, subY, W - contentX, L"Segoe UI", 12 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);

    // 4) 按钮
    for (auto& b : buttons)
        DrawButton(*this, b, th, s);

    // 5) 信息卡
    DrawCard();
}

void App::DrawCard() {
    float s = dpiScale;
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left;

    rt->FillRoundedRectangle(
        FzRR(cardX, cardY, cardX + cardW, cardY + cardH, 6 * s),
        MakeBrush(th.card).Get());
    rt->DrawRoundedRectangle(
        FzRR(cardX + 0.5f, cardY + 0.5f, cardX + cardW - 0.5f, cardY + cardH - 0.5f, 6 * s),
        MakeBrush(th.cardBorder).Get(), 1);
    // 强调色色块
    rt->FillRoundedRectangle(
        FzRR(cardX + 16 * s, cardY + 16 * s, cardX + 16 * s + 36 * s, cardY + 16 * s + 36 * s, 3 * s),
        MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
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
}

// ==================== 输入 ====================

void App::OnMove(float x, float y) {
    int hit = HitButton(buttons, x, y);
    for (int i = 0; i < (int)buttons.size(); i++)
        buttons[i].hot = (i == hit);
    // 导航 hover
    int nHit = HitNavItem(navGeo, navItems, x, y);
    for (int i = 0; i < (int)navItems.size(); i++)
        navItems[i].hot = (i == nHit);
    needsDraw = true;
}

void App::OnLButtonDown(float x, float y) {
    int hit = HitButton(buttons, x, y);
    if (hit >= 0) {
        buttons[hit].pressed = true;
        SetCapture(hwnd);
        needsDraw = true;
    }
}

void App::OnLButtonUp(float x, float y) {
    if (GetCapture() == hwnd) ReleaseCapture();
    int hit = HitButton(buttons, x, y);
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
    // 导航点击选中
    int nHit = HitNavItem(navGeo, navItems, x, y);
    if (nHit >= 0 && nHit != navSelected) {
        for (int i = 0; i < (int)navItems.size(); i++)
            navItems[i].selected = (i == nHit);
        navSelected = nHit;
        pageTitle = navItems[nHit].text;   // 右侧大标题跟随选中项
        RebuildDetail();
    }
    needsDraw = true;
}

// ==================== 动画 ====================

void App::Update(float dt) {
    animating = false;
    for (auto& b : buttons)
        if (UpdateButtonAnimation(b, dt))
            animating = true;
    for (auto& it : navItems)
        if (UpdateNavItemAnimation(it, dt))
            animating = true;
}

// ==================== 消息循环 ====================

void App::Run() {
    MSG msg;
    while (!quit) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { quit = true; break; }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (quit) break;
        float t = (float)GetTickCount64();
        float dt = Fzmn(0.1f, FzMx(0.0f, t - lastT) / 1000.0f);
        lastT = t;
        CheckThemeChange();
        Update(dt);
        if (needsDraw && rt) {
            Draw();
            needsDraw = false;
        }
// 动画进行中：短睡保证 ~60fps；空闲时限等 500ms（到期也醒来查一次系统主题，
            // 保证"运行中切换深浅主题"能被实时检测；空闲成本≈零，无消息即回阻塞）
            if (animating || needsDraw) Sleep(16);
            else MsgWaitForMultipleObjectsEx(0, nullptr, 500, QS_ALLINPUT, MWMO_ALERTABLE);
    }
}

// ==================== Win32 消息 ====================

LRESULT CALLBACK App::WndProcStatic(HWND h, UINT m, WPARAM w, LPARAM l) {
    App* self = nullptr;
    if (m == WM_NCCREATE) {
        auto* cs = (CREATESTRUCTW*)l;
        self = (App*)cs->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
    } else {
        self = (App*)GetWindowLongPtrW(h, GWLP_USERDATA);
    }
    if (self) return self->WndProc(h, m, w, l);
    return DefWindowProcW(h, m, w, l);
}

LRESULT App::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_NCCREATE:
        // 必须转发给 DefWindowProcW 完成窗口内部结构初始化，
        // 否则窗口破损、WM_CREATE 不会发出、CreateWindowExW 返回 NULL
        return DefWindowProcW(h, m, w, l);
    case WM_CREATE:
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps; BeginPaint(hwnd, &ps); EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto* mm = (MINMAXINFO*)l;
        mm->ptMinTrackSize.x = 560;
        mm->ptMinTrackSize.y = 420;
        return 0;
    }
    case WM_SIZE:
        if (LOWORD(l) > 0 && HIWORD(l) > 0)
            Resize(hwnd, LOWORD(l), HIWORD(l));
        Layout();
        return 0;
    case WM_MOUSEMOVE:
        OnMove(GET_X_LPARAM(l), GET_Y_LPARAM(l));
        {
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
        for (auto& it : navItems) it.hot = false;
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
        return 1;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

} // namespace fz
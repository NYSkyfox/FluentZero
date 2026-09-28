#include "pch.h"
#include "Core/App.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Platform/SystemSettings.h"
#include "UI/NavPane.h"
#include "pages/HomePage.h"
#include "pages/SettingsPage.h"
#include "pages/AccountsPage.h"
#include "pages/NetworkPage.h"

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

    // 导航项（必须先于 CreateWindowExW 填充：WM_SIZE 早到会 Layout→访问 navItems）
    navItems.push_back({ L"Home",     0xE80F, 0, 0, 0, false, true  });
    navItems.push_back({ L"Settings", 0xE713, 0, 0, 0, false, false });
    navItems.push_back({ L"Accounts", 0xE77B, 0, 0, 0, false, false });
    navItems.push_back({ L"Network",  0xE839, 0, 0, 0, false, false });

    // 页面：与 navItems 一一对应（同一顺序），一个页面对应一个文件
    pages.push_back(new HomePage);
    pages.push_back(new SettingsPage);
    pages.push_back(new AccountsPage);
    pages.push_back(new NetworkPage);

    hwnd = CreateWindowExW(0, wc.lpszClassName, L"FluentZero",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 780, 720,
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
    if (CurrentPage()) CurrentPage()->OnThemeChanged(*this);   // 初始化 Info 卡数据
    currentLight = th.light;
    lastThemePollMs = GetTickCount64();
    lastT = (float)GetTickCount64();
    return S_OK;
}

// ==================== 布局 ====================

void App::Layout() {
    if (navItems.empty()) return;
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left, H = rc.bottom - rc.top;
    float s = dpiScale;

    // ---- 左侧导航栏 ----
    navGeo.x = 0; navGeo.y = 0;
    navGeo.w = (48 + 132 * EaseOut(navState.t)) * s;   // 展开 180px ↔ 折叠 48px
    navGeo.h = H;
    float navTop = 48 * s;
    float navItemH = 40 * s;
    for (int i = 0; i < (int)navItems.size(); i++) {
        navItems[i].y = navTop + i * navItemH;
        navItems[i].h = navItemH;
    }

    // ---- 内容区（导航栏之后）+ 大标题 / 副标题 ----
    float m = 28 * s;
    contentX = navGeo.w + m;
    contentRegion.x = navGeo.w;
    contentRegion.y = 0;
    contentRegion.w = W - navGeo.w;
    contentRegion.h = H;
    contentRegion.s = s;

    float y = m;
    titleY = y;   y += 40 * s;
    subY   = y;   y += 30 * s;
    contentRegion.y = y;                    // 页面内容从大标题/副标题之下开始
    contentRegion.h = H - y;

    // 当前页面布局
    if (CurrentPage())
        CurrentPage()->Layout(*this, contentRegion);
    needsDraw = true;
}

// ==================== 主题 ====================

void App::CheckThemeChange() {
    ULONGLONG now = GetTickCount64();
    if (now - lastThemePollMs < 500) return;
    lastThemePollMs = now;
    bool lightNow = SystemUsesLightTheme(hwnd);
    if (lightNow != currentLight) {
        currentLight = lightNow;
        th = FluentTheme::Create(lightNow);
        if (CurrentPage()) CurrentPage()->OnThemeChanged(*this);
        needsDraw = true;
    }
}

// ==================== 绘制 ====================

void App::onDraw() {
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left, H = rc.bottom - rc.top;
    float s = dpiScale;

    // 1) 整窗清透明（磨砂只作用于左侧导航栏）
    rt->Clear(FzCol(0, 0, 0, 0));
    // 2) 右侧内容区实底
    D2D1_RECT_F cb{};
    cb.left = navGeo.w; cb.top = 0;
    cb.right = W;       cb.bottom = H;
    rt->FillRectangle(&cb, MakeBrush(th.contentBg).Get());
    // 3) 左侧导航栏
    DrawNavPane(*this, navGeo, navItems, navState, th, s);
    // 4) 大标题 + 副标题
    if (CurrentPage())
        DrawText(CurrentPage()->Title(), contentX, titleY, W - contentX,
                 L"Segoe UI", 26 * s, (DWRITE_FONT_WEIGHT)600, th.text1);
    DrawText(L"Windows 10 Fluent Design  |  Pure Win32 + Direct2D  |  Zero-dependency single exe",
             contentX, subY, W - contentX, L"Segoe UI", 12 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);
    // 5) 当前页面内容
    if (CurrentPage())
        CurrentPage()->Draw(*this);
}

// ==================== 输入 ====================

void App::OnMove(float x, float y) {
    // 折叠/展开按钮 + 导航项 hover（壳层职责）
    navState.btnHot = HitNavPaneButton(navGeo, x, y, dpiScale);
    int nh = navState.btnHot ? -1 : HitNavItem(navGeo, navItems, x, y);
    for (int i = 0; i < (int)navItems.size(); i++)
        navItems[i].hot = (i == nh);
    // 页面输入
    if (CurrentPage())
        CurrentPage()->OnMove(*this, x, y);
    needsDraw = true;
}

void App::OnLButtonDown(float x, float y) {
    // 折叠/展开按钮：切换目标态（动画由 UpdateNavState 驱动，期间每帧重布局）
    if (HitNavPaneButton(navGeo, x, y, dpiScale)) {
        navState.expanded = !navState.expanded;
        needsDraw = true;
        return;
    }
    // 导航点击选中 + 切换页面
    int nHit = HitNavItem(navGeo, navItems, x, y);
    if (nHit >= 0 && nHit != navSelected) {
        for (int i = 0; i < (int)navItems.size(); i++)
            navItems[i].selected = (i == nHit);
        navSelected = nHit;
        Layout();   // 切到目标页 → 重新布局该页
        if (CurrentPage()) CurrentPage()->OnThemeChanged(*this);   // 刷新依赖页标题的 Info
        needsDraw = true;
        return;
    }
    // 页面输入
    if (CurrentPage())
        CurrentPage()->OnLButtonDown(*this, x, y);
}

void App::OnLButtonUp(float x, float y) {
    if (GetCapture() == hwnd) ReleaseCapture();
    if (CurrentPage())
        CurrentPage()->OnLButtonUp(*this, x, y);
    needsDraw = true;
}

// ==================== 动画 ====================

void App::Update(float dt) {
    animating = false;
    for (auto& it : navItems)
        if (UpdateNavItemAnimation(it, dt))
            animating = true;
    // 导航栏折叠/展开：动画期间宽度逐帧变化 → 每帧重布局
    if (UpdateNavState(navState, dt)) {
        animating = true;
        Layout();
    }
    // 当前页面动画
    if (CurrentPage() && CurrentPage()->Update(*this, dt))
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
        if (CurrentPage()) CurrentPage()->OnResize(*this);   // 刷新依赖客户区的信息
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
        for (auto& it : navItems) it.hot = false;
        navState.btnHot = false;
        if (CurrentPage()) CurrentPage()->OnLeave(*this);
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
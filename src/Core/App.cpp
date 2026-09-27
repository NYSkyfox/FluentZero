#include "pch.h"
#include "Core/App.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"
#include "UI/Button.h"
#include "UI/NavPane.h"
#include "UI/CheckBox.h"
#include "UI/RadioButton.h"
#include "UI/ToggleSwitch.h"
#include "UI/ProgressBar.h"
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

    // CheckBox（第 2 个默认勾选，演示勾选态）
    checkboxes.push_back({ L"Animations",     false, 0, 0, 0, 0, 0, 0, 0, false, false });
    checkboxes.push_back({ L"Sounds",         true,  0, 0, 0, 0, 0, 0, 0, false, false });
    checkboxes.push_back({ L"Dark mode",      false, 0, 0, 0, 0, 0, 0, 0, false, false });

    // RadioButton（组内互斥，默认选 0）
    radios.push_back({ L"Automatic", true,  0, 0, 0, 0, 0, false, false });
    radios.push_back({ L"Light",     false, 0, 0, 0, 0, 0, false, false });
    radios.push_back({ L"Dark",      false, 0, 0, 0, 0, 0, false, false });

    // ToggleSwitch（第 1 个默认开）
    toggles.push_back({ L"Notifications",  true,  0, 0, 0, 0, 0, 0, false, false });
    toggles.push_back({ L"Do not disturb", false, 0, 0, 0, 0, 0, 0, false, false });

    // ProgressBar（演示自动推进）
    progressBars.push_back({ L"Syncing…", 0.6f, 0, 0, 0, 0, true });

    // ProgressRing（确定态，演示自动推进）
    progressRings.push_back({ L"Loading…", 0.4f, 0, 0, 0, 0, true });
    progressRings.push_back({ L"Uploading", 0.8f, 0, 0, 0, 0, true });

    // Slider（可拖拽）
    sliders.push_back({ L"Brightness", 0.70f, 0, 0, 0, 0, 0, false, false });
    sliders.push_back({ L"Volume",     0.45f, 0, 0, 0, 0, 0, false, false });

    // RatingControl（星级评分）
    ratings.push_back({ L"How do you rate this?", 4, 0, 0, 0, 0, 0 });
    ratings.push_back({ L"Recommend it?",         3, 0, 0, 0, 0, 0 });

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
    y += 34 * s + 20 * s;

    // 控件行 1：CheckBox ×3
    row1Y = y;
    {
        float cx = contentX, itemH = 28 * s;
        for (int i = 0; i < (int)checkboxes.size(); i++) {
            CheckBox& c = checkboxes[i];
            c.y = row1Y; c.h = itemH;
            float tw = Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 18 * s + 8 * s + tw;
            c.x = Fzmn(cx, contentX + cw);
            cx += c.w + 22 * s;
        }
    }
    y += 30 * s + 16 * s;

    // 控件行 2：ToggleSwitch ×2（左）
    row2Y = y;
    {
        float itemH = 28 * s;
        float tx = contentX;
        for (int i = 0; i < (int)toggles.size(); i++) {
            ToggleSwitch& c = toggles[i];
            c.y = row2Y; c.h = itemH;
            float tw = Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 40 * s + 8 * s + tw;
            c.x = Fzmn(tx, contentX + cw);
            tx += c.w + 24 * s;
        }
    }
    y += 30 * s + 16 * s;

    // 控件行 3：RadioButton ×3（左）
    row3Y = y;
    {
        float itemH = 28 * s;
        float rx = contentX;
        for (int i = 0; i < (int)radios.size(); i++) {
            RadioButton& c = radios[i];
            c.y = row3Y; c.h = itemH;
            float tw = Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 18 * s + 8 * s + tw;
            c.x = Fzmn(rx, contentX + cw);
            rx += c.w + 20 * s;
        }
    }
    y += 30 * s + 14 * s;

    // 进度条 + 进度环（并排）
    {
        ProgressBar& p = progressBars[0];
        p.y = y; p.h = 22 * s; p.x = contentX; p.w = Fzmn(cw * 0.42f, 260 * s);
    }
    for (int i = 0; i < (int)progressRings.size(); i++) {
        ProgressRing& rg = progressRings[i];
        float halfW = Fzmn(cw, 150 * s);
        rg.y = y; rg.h = 22 * s;
        rg.x = Fzmn(contentX + progressBars[0].w + 28 * s + i * (halfW + 12 * s), contentX + cw - halfW);
        rg.w = halfW;
    }
    y += 22 * s + 18 * s;

    // 控件行 4：Slider ×2
    row4Y = y;
    {
        float itemH = 30 * s, ix = contentX;
        float halfW = Fzmn(cw, 150 * s);
        for (int i = 0; i < (int)sliders.size(); i++) {
            Slider& c = sliders[i];
            c.y = row4Y; c.h = itemH;
            c.x = Fzmn(ix, contentX + cw - halfW);
            c.w = halfW;
            ix += halfW + 24 * s;
        }
    }
    y += 30 * s + 16 * s;

    // 控件行 5：RatingControl ×2
    row5Y = y;
    {
        float itemH = 30 * s, ix = contentX;
        float ratingW = 22 * s * 5 + 10 * s;   // 5 星宽
        for (int i = 0; i < (int)ratings.size(); i++) {
            RatingControl& c = ratings[i];
            c.y = row5Y; c.h = itemH;
            c.x = Fzmn(ix, contentX + cw - ratingW);
            c.w = ratingW;
            ix += ratingW + 30 * s;
        }
    }
    y += 30 * s + 16 * s;

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

    // 5) CheckBox
    for (auto& c : checkboxes)
        DrawCheckBox(*this, c, th, s);

    // 6) ToggleSwitch
    for (auto& t : toggles)
        DrawToggleSwitch(*this, t, th, s);

    // 7) RadioButton
    for (auto& rb : radios)
        DrawRadioButton(*this, rb, th, s);

    // 8) ProgressBar
    for (auto& p : progressBars)
        DrawProgressBar(*this, p, th, s);

    // 9) ProgressRing
    for (auto& rg : progressRings)
        DrawProgressRing(*this, rg, th, s);

    // 10) Slider
    for (auto& sl : sliders)
        DrawSlider(*this, sl, th, s);

    // 11) RatingControl
    for (auto& rtg : ratings)
        DrawRatingControl(*this, rtg, th, s);

    // 12) 信息卡
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
    int bh = HitButton(buttons, x, y);
    for (int i = 0; i < (int)buttons.size(); i++)
        buttons[i].hot = (i == bh);
    int nh = HitNavItem(navGeo, navItems, x, y);
    for (int i = 0; i < (int)navItems.size(); i++)
        navItems[i].hot = (i == nh);
    int ch = HitCheckBox(checkboxes, x, y);
    for (int i = 0; i < (int)checkboxes.size(); i++)
        checkboxes[i].hot = (i == ch);
    int th_ = HitToggleSwitch(toggles, x, y);
    for (int i = 0; i < (int)toggles.size(); i++)
        toggles[i].hot = (i == th_);
    int rh = HitRadioButton(radios, x, y);
    for (int i = 0; i < (int)radios.size(); i++)
        radios[i].hot = (i == rh);
    // Slider hover
    int sh = HitSlider(sliders, x, y);
    for (int i = 0; i < (int)sliders.size(); i++)
        sliders[i].hot = (i == sh);
    // 拖拽中：跟随指针更新 value
    if (sliderDragIndex >= 0 && sliderDragIndex < (int)sliders.size()) {
        Slider& d = sliders[sliderDragIndex];
        if (d.w > 1.0f) d.value = FzMx(0.0f, Fzmn(1.0f, (x - d.x) / d.w));
    }
    // RatingControl 悬停预览（命中星号 1..5，0 表示离开）
    for (auto& rtg : ratings)
        rtg.hover = (y >= rtg.y && y <= rtg.y + rtg.h) ? RatingStarAt(rtg, x, dpiScale) : 0;
    needsDraw = true;
}

void App::OnLButtonDown(float x, float y) {
    int hit = HitButton(buttons, x, y);
    if (hit >= 0) {
        buttons[hit].pressed = true;
        SetCapture(hwnd);
        needsDraw = true;
        return;
    }
    // Slider 按下：开始拖拽并立即设值
    int sh = HitSlider(sliders, x, y);
    if (sh >= 0 && sliders[sh].w > 1.0f) {
        sliderDragIndex = sh;
        sliders[sh].dragging = true;
        sliders[sh].value = FzMx(0.0f, Fzmn(1.0f, (x - sliders[sh].x) / sliders[sh].w));
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
    // CheckBox 点击切换
    int ch = HitCheckBox(checkboxes, x, y);
    if (ch >= 0) { checkboxes[ch].checked = !checkboxes[ch].checked; needsDraw = true; }
    // ToggleSwitch 点击切换
    int th_ = HitToggleSwitch(toggles, x, y);
    if (th_ >= 0) { toggles[th_].on = !toggles[th_].on; needsDraw = true; }
    // RadioButton 点击选中（组内互斥）
    int rh = HitRadioButton(radios, x, y);
    if (rh >= 0 && rh != radioSelected) {
        for (int i = 0; i < (int)radios.size(); i++)
            radios[i].selected = (i == rh);
        radioSelected = rh;
        needsDraw = true;
    }
    // Slider 结束拖拽
    if (sliderDragIndex >= 0 && sliderDragIndex < (int)sliders.size()) {
        sliders[sliderDragIndex].dragging = false;
        sliderDragIndex = -1;
    }
    // RatingControl 点击打分
    int rhg = HitRatingControl(ratings, x, y);
    if (rhg >= 0) {
        int star = RatingStarAt(ratings[rhg], x, dpiScale);
        if (star >= 1) { ratings[rhg].value = star; needsDraw = true; }
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
    for (auto& c : checkboxes)
        if (UpdateCheckBox(c, dt))
            animating = true;
    for (auto& t : toggles)
        if (UpdateToggleSwitch(t, dt))
            animating = true;
    for (auto& rb : radios)
        if (UpdateRadioButton(rb, dt))
            animating = true;
    // ProgressBar 自动推进（演示）
    for (auto& p : progressBars) {
        if (p.active) {
            p.value += dt * 0.35f;
            if (p.value > 1.0f) p.value = 0.0f;
            animating = true;
        }
    }
    // ProgressRing 自动推进（演示）
    for (auto& rg : progressRings) {
        if (rg.active) {
            rg.value += dt * 0.30f;
            if (rg.value > 1.0f) rg.value = 0.0f;
            animating = true;
        }
    }
    // Slider hover 动画
    for (auto& sl : sliders)
        if (UpdateSlider(sl, dt))
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
        for (auto& c : checkboxes) c.hot = false;
        for (auto& t : toggles) t.hot = false;
        for (auto& rb : radios) rb.hot = false;
        for (auto& sl : sliders) sl.hot = false;
        for (auto& rtg : ratings) rtg.hover = 0;
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
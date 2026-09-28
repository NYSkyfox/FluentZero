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
    // 按钮三种样式各展示一次：标准 / Primary / Subtle（演示用，图标码位 0 纯文字）
    buttons.push_back({ L"Button",  0, false, false });
    buttons.push_back({ L"Primary", 0, true,  false });
    buttons.push_back({ L"Subtle",  0, false, true  });

    // 导航项（同样必须先于 CreateWindowExW 填充，防 WM_SIZE 早到越界）
    navItems.push_back({ L"Home",     0xE80F, 0, 0, 0, false, true  });
    navItems.push_back({ L"Settings", 0xE713, 0, 0, 0, false, false });
    navItems.push_back({ L"Accounts", 0xE77B, 0, 0, 0, false, false });
    navItems.push_back({ L"Network",  0xE839, 0, 0, 0, false, false });
    pageTitle = L"Home";

    // CheckBox 两态各一次：未勾选 / 已勾选
    checkboxes.push_back({ L"Animations", false, 0, 0, 0, 0, 0, 0, 0, false, false });
    checkboxes.push_back({ L"Sounds",     true,  0, 0, 0, 0, 0, 0, 0, false, false });

    // RadioButton 两态各一次：选中 / 未选中（组内互斥，默认选 0）
    radios.push_back({ L"Light", true,  0, 0, 0, 0, 0, false, false });
    radios.push_back({ L"Dark",  false, 0, 0, 0, 0, 0, false, false });

    // ToggleSwitch 三态各一次：On / Off / Disabled
    toggles.push_back({ L"Notifications",   true,  0, 0, 0, 0, 0, 0, false, false });
    toggles.push_back({ L"Do not disturb",  false, 0, 0, 0, 0, 0, 0, false, false });
    toggles.push_back({ L"Airplane mode",   false, 0, 0, 0, 0, 0, 0, false, false, true });

    // ProgressBar（单一样式，演示自动推进）
    progressBars.push_back({ L"Syncing...", 0.6f, 0, 0, 0, 0, true });

    // ProgressRing（单一样式，演示自动推进）
    progressRings.push_back({ L"Loading...", 0.4f, 0, 0, 0, 0, true });

    // Slider（单一样式，可拖拽）
    sliders.push_back({ L"Brightness", 0.70f, 0, 0, 0, 0, 0, false, false });

    // RatingControl（单一样式，星级评分）
    ratings.push_back({ L"How do you rate this?", 4, 0, 0, 0, 0, 0 });

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
    if (buttons.size() < 3 || navItems.empty()) return;   // 防御：未就绪前不布局（3 种按钮样式）
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left, H = rc.bottom - rc.top;
    float s = dpiScale;

    // ---- 左侧导航栏 ----
    navGeo.x = 0; navGeo.y = 0;
    // 宽度随折叠动画插值：展开 180px ↔ 折叠 48px（仅图标）
    navGeo.w = (48 + 132 * EaseOut(navState.t)) * s;
    navGeo.h = H;
    float navTop = 48 * s;      // 顶部留白（给窗体标题区）
    float navItemH = 40 * s;   // 2018 NavigationView 规范行高
    for (int i = 0; i < (int)navItems.size(); i++) {
        navItems[i].y = navTop + i * navItemH;
        navItems[i].h = navItemH;
    }

    // ---- 右侧内容区（两栏 + 分组标题）----
    float m = 28 * s;
    contentX = navGeo.w + m;
    float cw = W - contentX - m;          // 内容区可用宽度
    float gapCol = 28 * s;                 // 两栏间距
    colW = (cw - gapCol) * 0.5f;           // 每栏宽
    colLX = contentX;
    colRX = contentX + colW + gapCol;      // 右栏 X

    float y = m;
    titleY = y;   y += 40 * s;
    subY   = y;   y += 30 * s;
    float y0 = y;                          // 两栏顶部（对齐）

    // ========== 左栏：Buttons / Selection ==========
    {
        float ly = y0;
        gL1Y = ly;  ly += 16 * s + 10 * s;            // "Buttons"
        for (int i = 0; i < (int)buttons.size(); i++) { // 3 种样式各一次（竖排，全栏宽）
            Button& b = buttons[i];
            b.x = colLX; b.y = ly; b.h = 34 * s; b.w = colW;
            ly += 34 * s + (i < (int)buttons.size() - 1 ? 10 * s : 18 * s);
        }
        gL2Y = ly;  ly += 16 * s + 10 * s;            // "Selection"
        for (int i = 0; i < (int)checkboxes.size(); i++) {   // CheckBox ×3（竖排）
            CheckBox& c = checkboxes[i];
            c.x = colLX; c.y = ly; c.h = 28 * s;
            float tw = Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 20 * s + 8 * s + tw + 84 * s;   // 复选框 20px + 状态后缀（Unchecked）预留
            ly += 28 * s + (i < (int)checkboxes.size() - 1 ? 8 * s : 12 * s);
        }
        for (int i = 0; i < (int)toggles.size(); i++) {      // ToggleSwitch ×2（竖排）
            ToggleSwitch& c = toggles[i];
            c.x = colLX; c.y = ly; c.h = 28 * s;
            float tw = Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 40 * s + 8 * s + tw + 48 * s;   // 轨道 40px + 状态后缀（On/Off）预留
            ly += 28 * s + (i < (int)toggles.size() - 1 ? 8 * s : 12 * s);
        }
        for (int i = 0; i < (int)radios.size(); i++) {       // RadioButton ×3（竖排）
            RadioButton& c = radios[i];
            c.x = colLX; c.y = ly; c.h = 28 * s;
            float tw = Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 20 * s + 8 * s + tw + 84 * s;   // 单选按钮 20px + 状态后缀（Not selected）预留
            ly += 28 * s + (i < (int)radios.size() - 1 ? 6 * s : 0);
        }
    }

    // ========== 右栏：Progress / Sliders / Rating / Info ==========
    {
        float ry = y0;
        gR1Y = ry;  ry += 16 * s + 10 * s;            // "Progress"
        ProgressBar& p = progressBars[0];
        p.x = colRX; p.y = ry; p.h = 26 * s; p.w = colW;   // 加高：label 一行 + 下方轨道一行
        ry += 26 * s + 12 * s;
        for (int i = 0; i < (int)progressRings.size(); i++) {   // ProgressRing（全宽）
            ProgressRing& rg = progressRings[i];
            rg.x = colRX;
            rg.y = ry; rg.h = 22 * s; rg.w = colW;
        }
        ry += 22 * s + 16 * s;
        gR2Y = ry;  ry += 16 * s + 10 * s;            // "Sliders"
        for (int i = 0; i < (int)sliders.size(); i++) {     // Slider ×2（竖排，全栏宽）
            Slider& c = sliders[i];
            c.x = colRX; c.y = ry; c.h = 30 * s; c.w = colW;
            ry += 30 * s + (i < (int)sliders.size() - 1 ? 12 * s : 16 * s);
        }
        gR3Y = ry;  ry += 16 * s + 10 * s;            // "Rating"
        {
            float ratingW = FzMx(20 * s * 5 + 10 * s, 160 * s);  // max(5 星宽, 标签+星级行宽)
            for (int i = 0; i < (int)ratings.size(); i++) {
                RatingControl& c = ratings[i];
                c.x = colRX; c.y = ry; c.h = 30 * s; c.w = ratingW;
                ry += 30 * s + (i < (int)ratings.size() - 1 ? 12 * s : 16 * s);
            }
        }
        gR4Y = ry;  ry += 16 * s + 10 * s;            // "Info" + Card
        cardX = colRX; cardY = ry;
        cardW = colW;
        // 卡片需容纳 6 行信息：6*24(行) + 上下 padding(16*2) + 底部小字(约 18) + 余量
        const float cardMin = (kDetailRows * 24 + 16 * 2 + 18 + 20) * s;
        cardH = FzMx(cardMin, H - ry - m);
    }
    needsDraw = true;
}

void App::RebuildDetail() {
    // 6 行运行时信息（标签/值分离，DrawCard 用固定 X 网格两列绘制，不再靠空格对齐）
    RECT rc; GetClientRect(hwnd, &rc);
    const int cw = rc.right - rc.left, chh = rc.bottom - rc.top;
    const int dpiPct = (int)(dpiScale * 100.0f + 0.5f);

    detailLabel[0] = L"System theme";
    detailValue[0] = th.light ? L"Light" : L"Dark";
    detailLabel[1] = L"Accent color";
    detailValue[1] = HexOf(th.accent);
    detailLabel[2] = L"DPI scale";
    detailValue[2] = std::to_wstring(dpiPct) + L"% (" +
                     std::to_wstring((int)(dpiScale * 96.0f + 0.5f)) + L" DPI)";
    detailLabel[3] = L"Client size";
    detailValue[3] = std::to_wstring(cw) + L" x " + std::to_wstring(chh) + L" px";
    detailLabel[4] = L"Current page";
    detailValue[4] = pageTitle;
    detailLabel[5] = L"Primary clicks";
    detailValue[5] = std::to_wstring(primaryClicks);
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
    float W = rc.right - rc.left, H = rc.bottom - rc.top;
    float s = dpiScale;

    // 1) 整窗清为全透明（磨砂只作用于左侧导航栏；内容区随后画实底覆盖，
    //    Win10 风格：内容区不透出后方，只有侧边栏毛玻璃）
    rt->Clear(FzCol(0, 0, 0, 0));

    // 2) 右侧内容区：实底背景（从导航栏右缘画到窗口右缘，无缝、不透出后方）
    D2D1_RECT_F cb{};
    cb.left = navGeo.w; cb.top = 0;
    cb.right = W;       cb.bottom = H;
    rt->FillRectangle(&cb, MakeBrush(th.contentBg).Get());

    // 3) 左侧导航栏（磨砂面板 + 顶部标题/折叠按钮 + Reveal 交互，半透明透出后方 = Acrylic）
    DrawNavPane(*this, navGeo, navItems, navState, th, s);

    // 4) 右侧内容区：标题 / 副标题
    DrawText(pageTitle, contentX, titleY, W - contentX, L"Segoe UI", 26 * s,
             (DWRITE_FONT_WEIGHT)600, th.text1);
    DrawText(L"Windows 10 Fluent Design  |  Pure Win32 + Direct2D  |  Zero-dependency single exe",
             contentX, subY, W - contentX, L"Segoe UI", 12 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);

    // 5) 分组标题（两栏分节，对齐 Win10 设置页）
    DrawText(L"BUTTONS",   colLX, gL1Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    DrawText(L"SELECTION", colLX, gL2Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    DrawText(L"PROGRESS",  colRX, gR1Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    DrawText(L"SLIDERS",   colRX, gR2Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    DrawText(L"RATING",    colRX, gR3Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    DrawText(L"INFO",      colRX, gR4Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);

    // 6) 左栏控件：按钮 / CheckBox / ToggleSwitch / RadioButton
    for (auto& b : buttons)
        DrawButton(*this, b, th, s);
    for (auto& c : checkboxes)
        DrawCheckBox(*this, c, th, s);
    for (auto& t : toggles)
        DrawToggleSwitch(*this, t, th, s);
    for (auto& rb : radios)
        DrawRadioButton(*this, rb, th, s);

    // 7) 右栏控件：ProgressBar / ProgressRing / Slider / RatingControl
    for (auto& p : progressBars)
        DrawProgressBar(*this, p, th, s);
    for (auto& rg : progressRings)
        DrawProgressRing(*this, rg, th, s);
    for (auto& sl : sliders)
        DrawSlider(*this, sl, th, s);
    for (auto& rtg : ratings)
        DrawRatingControl(*this, rtg, th, s);

    // 8) 信息卡
    DrawCard();
}

void App::DrawCard() {
    float s = dpiScale;

    rt->FillRoundedRectangle(
        FzRR(cardX, cardY, cardX + cardW, cardY + cardH, 6 * s),
        MakeBrush(th.card).Get());
    rt->DrawRoundedRectangle(
        FzRR(cardX + 0.5f, cardY + 0.5f, cardX + cardW - 0.5f, cardY + cardH - 0.5f, 6 * s),
        MakeBrush(th.cardBorder).Get(), 1);
    // 内容块相对卡片垂直居中（不再固定顶部偏移 → 图标与三行文本整体对齐）
    const float pad = 16 * s;
    const float lineH = 24 * s, capH = 3 * s;
    const float blockH = kDetailRows * lineH;
    float blockTop = cardY + FzMx(pad, (cardH - blockH) * 0.5f);
    // 强调色色块：相对三行文本块垂直居中
    rt->FillRoundedRectangle(
        FzRR(cardX + pad, blockTop + (blockH - 36 * s) * 0.5f,
             cardX + pad + 36 * s, blockTop + (blockH - 36 * s) * 0.5f + 36 * s, 3 * s),
        MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
    // 双列固定 X 网格：标签列 / 值列（值列等宽字体右对齐，三行严格竖直对齐）
    float lx = cardX + pad + 36 * s + 16 * s;
    float labelW = 132 * s;
    float colW2 = Fzmn(140 * s, cardX + cardW - pad - (lx + labelW));
    float vx = lx + labelW + 8 * s;
    for (int i = 0; i < kDetailRows; i++) {
        float ty = blockTop + i * lineH + (lineH - capH) * 0.5f;   // 行内文本垂直居中
        DrawText(detailLabel[i], lx, ty, labelW, L"Segoe UI", 13 * s,
                 DWRITE_FONT_WEIGHT_NORMAL, th.text2);
        DrawText(detailValue[i], vx, ty, colW2, L"Consolas", 13 * s,
                 DWRITE_FONT_WEIGHT_NORMAL, th.text1);
    }
    // 底部小字（12px，可读性；文案需在可用宽度内，过长会被硬截断）
    DrawText(L"Reveal hover 150ms | Segoe MDL2 | Acrylic",
             cardX + pad, cardY + cardH - pad - capH, cardW - 2 * pad, L"Segoe UI", 12 * s,
             DWRITE_FONT_WEIGHT_NORMAL, th.text2);
}

// ==================== 输入 ====================

void App::OnMove(float x, float y) {
    int bh = HitButton(buttons, x, y);
    for (int i = 0; i < (int)buttons.size(); i++)
        buttons[i].hot = (i == bh);
    // 折叠/展开按钮 hover（优先于导航项命中）
    navState.btnHot = HitNavPaneButton(navGeo, x, y, dpiScale);
    int nh = navState.btnHot ? -1 : HitNavItem(navGeo, navItems, x, y);
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
    // 折叠/展开按钮：切换目标态（动画由 UpdateNavState 驱动，期间每帧重布局）
    if (HitNavPaneButton(navGeo, x, y, dpiScale)) {
        navState.expanded = !navState.expanded;
        needsDraw = true;
        return;
    }
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
    // 导航栏折叠/展开：动画期间宽度逐帧变化 → 每帧重布局（内容区随之平移）
    if (UpdateNavState(navState, dt)) {
        animating = true;
        Layout();
    }
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
        if (hwnd) RebuildDetail();   // 刷新 Info 卡的 Client size
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
        navState.btnHot = false;
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
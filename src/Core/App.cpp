#include "pch.h"
#include "Core/App.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"
#include "UI/Button.h"
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

    hwnd = CreateWindowExW(0, wc.lpszClassName, L"FluentZero",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 520,
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
    lastT = (float)GetTickCount64();
    return S_OK;
}

// ==================== 布局 ====================

void App::Layout() {
    if (buttons.size() < 4) return;   // 防御：buttons 未就绪前不布局
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
    float ptw = Measure(pb.text, L"Segoe UI", 13 * s, (DWRITE_FONT_WEIGHT)600);
    pb.w = ptw + 18 * s + 6 * s + 32 * s;
    y += 34 * s + 24 * s;

    cardX = m; cardY = y;
    cardW = W - 2 * m;
    cardH = FzMx(110 * s, H - y - m);
    needsDraw = true;
}

void App::RebuildDetail() {
    detailAccent = L"Accent color (system)   " + HexOf(th.accent);
    detailTheme  = th.light ? L"System theme            Light" : L"System theme            Dark";
    detailClicks = L"Primary clicked         " + std::to_wstring(primaryClicks) + L" time(s)";
    needsDraw = true;
}

// ==================== 绘制 ====================

void App::onDraw() {
    RECT rc; GetClientRect(hwnd, &rc);
    float W = rc.right - rc.left;
    float s = dpiScale;

    // 1) 背景：Acrylic 半透明主题色
    rt->Clear(Premul(th.bg));

    // 2) 标题 / 副标题
    DrawText(L"FluentZero", 28 * s, titleY, W, L"Segoe UI", 26 * s,
             (DWRITE_FONT_WEIGHT)600, th.text1);
    DrawText(L"Windows 10 Fluent Design · 纯 Win32 + Direct2D 手搓 · 零依赖单 exe",
             28 * s, subY, W, L"Segoe UI", 12 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);

    // 3) 按钮
    for (auto& b : buttons)
        DrawButton(*this, b, th, s);

    // 4) 信息卡
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
    needsDraw = true;
}

// ==================== 动画 ====================

void App::Update(float dt) {
    animating = false;
    for (auto& b : buttons)
        if (UpdateButtonAnimation(b, dt))
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
        Update(dt);
        if (needsDraw && rt) {
            Draw();
            needsDraw = false;
        }
        // 动画进行中：短睡保证 ~60fps；空闲时阻塞等待消息（零 CPU）
        if (animating || needsDraw) Sleep(16);
        else WaitMessage();
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
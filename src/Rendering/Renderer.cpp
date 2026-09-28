#include "pch.h"
#include "Renderer.h"
#include "Utils/ColorUtils.h"

namespace fz {

HRESULT Renderer::Init(HWND hwnd, int w, int h) {
    // 1) 窗口不重定向位图：内容直接交给 DWM 合成（保留原生标题栏）
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (!(ex & WS_EX_NOREDIRECTIONBITMAP)) {
        SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex | WS_EX_NOREDIRECTIONBITMAP);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    // 2) Acrylic：DWM 模糊窗口后方内容（Win10 1607+ 经典实现）
    DWM_BLURBEHIND bb{};
    bb.dwFlags = DWM_BB_ENABLE;
    bb.fEnable = TRUE;
    if (SUCCEEDED(DwmEnableBlurBehindWindow(hwnd, &bb))) ok = true;
    // 3) D2D1 工厂（1.0 只有 SINGLE / MULTI_THREADED 两种）
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, d2dFactory.GetAddressOf())))
        return E_FAIL;
    // 4) 手搓渲染目标属性（不依赖 d2d1helper.h 的 C++ 辅助函数）
    D2D1_RENDER_TARGET_PROPERTIES rtp{};
    rtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    rtp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    rtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    rtp.dpiX = 96.0f; rtp.dpiY = 96.0f;
    rtp.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    rtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    D2D1_HWND_RENDER_TARGET_PROPERTIES hrp{};
    hrp.hwnd = hwnd;
    hrp.pixelSize.width = (UINT32)w; hrp.pixelSize.height = (UINT32)h;
    hrp.presentOptions = D2D1_PRESENT_OPTIONS_NONE;
    if (FAILED(d2dFactory->CreateHwndRenderTarget(rtp, hrp, &rt)))
        return E_FAIL;
    // 5) DirectWrite
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory), (IUnknown**)dw.GetAddressOf())))
        return E_FAIL;
    return S_OK;
}

void Renderer::Resize(HWND hwnd, int w, int h) {
    if (rt && w > 1 && h > 1) {
        D2D1_SIZE_U su{}; su.width = (UINT32)w; su.height = (UINT32)h;
        rt->Resize(&su);
    }
}

void Renderer::Draw() {
    if (!rt) return;
    rt->BeginDraw();
    onDraw();
    rt->EndDraw();
}

// ---------------------- 文本绘制工具 ----------------------

ComPtr<ID2D1SolidColorBrush> Renderer::MakeBrush(D2D1_COLOR_F c) {
    ComPtr<ID2D1SolidColorBrush> b;
    rt->CreateSolidColorBrush(Premul(c), &b);
    return b;
}

void Renderer::DrawText(const std::wstring& t, float x, float y, float maxW,
                        const wchar_t* face, float size, DWRITE_FONT_WEIGHT weight, D2D1_COLOR_F c,
                        DWRITE_TEXT_ALIGNMENT align, DWRITE_PARAGRAPH_ALIGNMENT vAlign, float maxH) {
    if (!dw) return;
    ComPtr<IDWriteTextFormat> f;
    if (FAILED(dw->CreateTextFormat(face, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &f)))
        return;
    f->SetTextAlignment(align);
    f->SetParagraphAlignment(vAlign);
    ComPtr<IDWriteTextLayout> lay;
    if (FAILED(dw->CreateTextLayout(t.c_str(), (UINT32)t.size(), f.Get(), maxW, maxH, &lay)))
        return;
    D2D1_POINT_2F p{};
    p.x = x; p.y = y;
    rt->DrawTextLayout(p, lay.Get(), MakeBrush(c).Get());
}

float Renderer::Measure(const std::wstring& t, const wchar_t* face, float size, DWRITE_FONT_WEIGHT weight) {
    if (!dw) return (float)t.size() * size * 0.6f;
    ComPtr<IDWriteTextFormat> f;
    dw->CreateTextFormat(face, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &f);
    if (!f) return (float)t.size() * size * 0.6f;
    ComPtr<IDWriteTextLayout> lay;
    if (FAILED(dw->CreateTextLayout(t.c_str(), (UINT32)t.size(), f.Get(), 1e6f, 1e6f, &lay)))
        return (float)t.size() * size * 0.6f;
    DWRITE_TEXT_METRICS tm{};
    lay->GetMetrics(&tm);
    return tm.layoutWidth;
}

} // namespace fz
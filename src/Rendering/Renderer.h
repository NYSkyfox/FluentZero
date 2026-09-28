#pragma once

// 渲染后端：D2D1 HwndRenderTarget + DirectWrite
// Win32 经典路径：WS_EX_NOREDIRECTIONBITMAP + DWM BlurBehind（Acrylic）
// 依赖：Utils/ColorUtils.h
namespace fz {

struct Renderer {
    ComPtr<ID2D1Factory> d2dFactory;
    ComPtr<ID2D1HwndRenderTarget> rt;
    ComPtr<IDWriteFactory> dw;
    bool ok = false;

    // 初始化：设置 NORedirectionBitmap + Acrylic + D2D1 工厂 + HwndRT + DirectWrite
    HRESULT Init(HWND hwnd, int w, int h);
    // 窗口尺寸变化时同步 RenderTarget
    void Resize(HWND hwnd, int w, int h);
    // 执行一帧绘制（BeginDraw → onDraw → EndDraw）
    void Draw();

    // 子类实现具体绘制
    virtual void onDraw() = 0;

    // ---- 文本绘制工具（供子类使用）----
    // 创建纯色画刷（自动预乘 alpha）
    ComPtr<ID2D1SolidColorBrush> MakeBrush(D2D1_COLOR_F c);
    // 在 (x,y) 处绘制文本
    // align: 水平对齐（默认左对齐）
    // vAlign: 垂直对齐（默认顶部；传 CENTER 时在 [y, y+maxH] 内垂直居中）
    // maxH: 布局框高度（默认 1e6=不限；垂直居中时传实际可用高度如按钮高度）
    void DrawText(const std::wstring& t, float x, float y, float maxW,
                  const wchar_t* face, float size, DWRITE_FONT_WEIGHT weight, D2D1_COLOR_F c,
                  DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING,
                  DWRITE_PARAGRAPH_ALIGNMENT vAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                  float maxH = 1e6f);
    // 测量文本宽度（用于按钮自动宽度）
    float Measure(const std::wstring& t, const wchar_t* face, float size, DWRITE_FONT_WEIGHT weight);
};

} // namespace fz
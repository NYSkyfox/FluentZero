#pragma once

// Reveal 按钮（Win10 Fluent 风格）
// 依赖：Utils（MathUtils/ColorUtils）、Theme/FluentTheme.h、Rendering/Renderer.h
namespace fz {

// 前向声明（.cpp 中再 include 完整头）
struct Renderer;
struct FluentTheme;

// 按钮状态 + 动画进度
struct Button {
    std::wstring text;
    UINT32 glyph = 0;      // Segoe MDL2 Assets 码位，0 = 无图标
    bool primary = false;  // 强调色（accent 实底）
    bool subtle = false;   // 轻量（无填充无边框，仅 hover 淡底）
    bool disabled = false; // 禁用（灰态，不可交互）

    // 逻辑坐标（96dpi 基准，绘制时按 dpiScale 缩放）
    float x = 0, y = 0, w = 0, h = 0;

    // 动画进度 0..1
    float hoverT = 0, pressT = 0, revealT = 0;
    bool hot = false, pressed = false;
};

// 按钮绘制 / 命中 / 动画 独立函数（与具体 App 解耦）
// 绘制一个按钮（需要 rt + dw 已通过 Renderer 提供）
void DrawButton(Renderer& r, const Button& b, const FluentTheme& th, float dpiScale);
// 命中检测：返回命中的按钮下标，未命中返回 -1
int HitButton(const std::vector<Button>& buttons, float x, float y);
// 推进一个按钮的动画（dt 秒），返回是否仍在动画中
bool UpdateButtonAnimation(Button& b, float dt);

} // namespace fz
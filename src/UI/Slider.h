#pragma once

// Slider（Win10 Fluent 风格，水平滑块，可拖拽 thumb）
// 灰色轨道 + accent 已选段 + 白色圆 thumb（中心 accent 点）；value 0..1
namespace fz {

struct Renderer;
struct FluentTheme;

struct Slider {
    std::wstring label;
    float value = 0.5f;   // 0..1
    float x = 0, y = 0, w = 0, h = 0;
    float hoverT = 0;     // thumb hover/按下 渐显
    bool hot = false;     // 命中
    bool dragging = false;// 拖拽中
};

void DrawSlider(Renderer& r, const Slider& s, const FluentTheme& th, float dpiScale);
int HitSlider(const std::vector<Slider>& v, float x, float y);
bool UpdateSlider(Slider& s, float dt);

} // namespace fz
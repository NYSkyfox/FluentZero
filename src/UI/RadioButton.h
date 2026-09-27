#pragma once

// RadioButton（Win10 Fluent 风格，组内互斥由调用方管理）
// 外圈描边 + hover 内环 + 选中中心 accent 点
namespace fz {

struct Renderer;
struct FluentTheme;

struct RadioButton {
    std::wstring label;
    bool selected = false;
    float x = 0, y = 0, w = 0, h = 0;
    float hoverT = 0;
    bool hot = false, pressed = false;
};

void DrawRadioButton(Renderer& r, const RadioButton& rb, const FluentTheme& th, float dpiScale);
int HitRadioButton(const std::vector<RadioButton>& v, float x, float y);
bool UpdateRadioButton(RadioButton& rb, float dt);

} // namespace fz
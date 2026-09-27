#pragma once

// ToggleSwitch（Win10 Fluent 风格）
// 滑块轨道 + 圆钮，开/关状态，圆钮滑动 + 轨道变色动画
namespace fz {

struct Renderer;
struct FluentTheme;

struct ToggleSwitch {
    std::wstring label;
    bool on = false;
    float x = 0, y = 0, w = 0, h = 0;
    float hoverT = 0, toggleT = 0;   // toggleT：开/关动画进度
    bool hot = false, pressed = false;
    bool disabled = false;        // 禁用态（灰化、不可交互）
};

void DrawToggleSwitch(Renderer& r, const ToggleSwitch& ts, const FluentTheme& th, float dpiScale);
int HitToggleSwitch(const std::vector<ToggleSwitch>& v, float x, float y);
bool UpdateToggleSwitch(ToggleSwitch& ts, float dt);

} // namespace fz
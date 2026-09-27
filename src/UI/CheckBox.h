#pragma once

// CheckBox（Win10 Fluent 风格）
// 方框 + 勾选（accent 填充 + 白色勾），hover 边框提亮，勾选带淡入动画
// 依赖：Utils、Theme、Rendering
namespace fz {

struct Renderer;
struct FluentTheme;

struct CheckBox {
    std::wstring label;
    bool checked = false;
    float x = 0, y = 0, w = 0, h = 0;
    float hoverT = 0, pressT = 0, checkT = 0;   // checkT：勾选淡入动画
    bool hot = false, pressed = false;
};

void DrawCheckBox(Renderer& r, const CheckBox& cb, const FluentTheme& th, float dpiScale);
int HitCheckBox(const std::vector<CheckBox>& v, float x, float y);
bool UpdateCheckBox(CheckBox& cb, float dt);

} // namespace fz
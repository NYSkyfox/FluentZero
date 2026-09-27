#pragma once

// ProgressBar（Win10 Fluent 风格，决定态）
// 灰色轨道 + accent 填充，value 0..1，点击循环推进演示
namespace fz {

struct Renderer;
struct FluentTheme;

struct ProgressBar {
    std::wstring label;
    float value = 0;      // 0..1
    float x = 0, y = 0, w = 0, h = 0;
    bool active = true;   // 是否自动推进（演示用）
};

void DrawProgressBar(Renderer& r, const ProgressBar& pb, const FluentTheme& th, float dpiScale);
int HitProgressBar(const std::vector<ProgressBar>& v, float x, float y);

} // namespace fz
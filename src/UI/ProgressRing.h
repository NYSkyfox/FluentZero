#pragma once

// ProgressRing（Win10 Fluent 风格，不确定/确定态进度环）
// 灰色底环 + accent 前景弧（从顶部顺时针），value 0..1
namespace fz {

struct Renderer;
struct FluentTheme;

struct ProgressRing {
    std::wstring label;
    float value = 0;      // 0..1
    float x = 0, y = 0, w = 0, h = 0;
    bool active = true;   // 是否自动推进（演示用）
};

void DrawProgressRing(Renderer& r, const ProgressRing& pr, const FluentTheme& th, float dpiScale);

} // namespace fz
#pragma once

// RatingControl（Win10 Fluent 风格，星级评分）
// 5 颗星，value 0..5，点击第 k 星 = k 分，hover 时预览
namespace fz {

struct Renderer;
struct FluentTheme;

struct RatingControl {
    std::wstring label;
    int value = 0;      // 0..5
    int hover = 0;      // 悬停预览星数（0 表示无）
    float x = 0, y = 0, w = 0, h = 0;
};

void DrawRatingControl(Renderer& r, const RatingControl& rc, const FluentTheme& th, float dpiScale);
int HitRatingControl(const std::vector<RatingControl>& v, float x, float y);
// 返回 x 命中的星号（1..5），0 表示未命中任何星
int RatingStarAt(const RatingControl& rc, float x, float dpiScale);

} // namespace fz
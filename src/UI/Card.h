#pragma once

// Card 控件（Win10 Fluent 风格，圆角底色 + 1px 描边）
// 用法：配置 Card 参数 → 调用 DrawCard()
namespace fz {

struct Renderer;
struct FluentTheme;

// 卡片控件（Card 控件）
struct Card {
    std::wstring title;          // 标题（可选）
    std::wstring content;        // 内容文字（支持换行，用 L"\\n" 分隔）
    float x = 0, y = 0, w = 0, h = 0;
    bool elevated = false;       // 是否显示轻微投影
    float elevationT = 0;        // 投影动画进度 0..1
};

// 绘制卡片控件
void DrawCardWidget(Renderer& r, const Card& c, const FluentTheme& th, float dpiScale);

} // namespace fz
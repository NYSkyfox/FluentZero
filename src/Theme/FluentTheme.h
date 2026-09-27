#pragma once

// Fluent Design 主题（颜色方案）
// 依赖：Utils/ColorUtils.h（D2D1_COLOR_F）、Platform/SystemSettings.h
namespace fz {

struct FluentTheme {
    bool light = true;
    D2D1_COLOR_F bg;            // Acrylic 底色（带 alpha，模糊从透明处透出）
    D2D1_COLOR_F card;
    D2D1_COLOR_F cardBorder;
    D2D1_COLOR_F text1;         // 主文字
    D2D1_COLOR_F text2;         // 次级文字
    D2D1_COLOR_F textOnAccent;
    D2D1_COLOR_F accent;
    D2D1_COLOR_F btnFill;
    D2D1_COLOR_F btnBorder;
    D2D1_COLOR_F btnText;
    D2D1_COLOR_F reveal;        // Reveal 描边颜色
    D2D1_COLOR_F navPane;       // 侧边导航磨砂底色
    D2D1_COLOR_F navBorder;     // 侧边导航右侧分割线
    D2D1_COLOR_F navHover;      // 导航 hover 灰条
    D2D1_COLOR_F navSweep;      // 导航 Reveal 光带

    // 从系统设置（强调色 + 深浅主题）构建完整主题
    static FluentTheme Create();
    // 纯映射：给定深浅（light）+ 当前系统强调色，产出对应色板（不读系统）
    static FluentTheme Create(bool light);
};

} // namespace fz
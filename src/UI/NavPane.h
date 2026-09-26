#pragma once

// 侧边导航栏（Win10 1809+ 设置导航风格）
// 磨砂面板底色（Acrylic 透出）+ hover 灰条渐显 + Reveal 光带扫过 + 强调色选中指示条
// 依赖：Utils（MathUtils/ColorUtils）、Theme/FluentTheme.h、Rendering/Renderer.h
namespace fz {

// 前向声明（.cpp 中再 include 完整头）
struct Renderer;
struct FluentTheme;

// 单个导航项
struct NavItem {
    std::wstring text;
    UINT32 glyph = 0;      // Segoe MDL2 Assets 码位
    float y = 0, h = 0;    // 逻辑坐标（由 Layout 计算）
    float hoverT = 0;      // hover 动画进度 0..1
    bool hot = false;
    bool selected = false;
};

// 导航栏几何状态（由 App::Layout 计算）
struct NavGeometry {
    float x = 0, y = 0, w = 0, h = 0;   // 面板矩形
};

// 绘制导航栏（磨砂面板 + 各导航项：hover 条 / Reveal 光带 / 选中指示条 / 图标文字）
void DrawNavPane(Renderer& r, const NavGeometry& geo, const std::vector<NavItem>& items,
                 const FluentTheme& th, float dpiScale);
// 命中检测：返回命中的导航项下标，未命中返回 -1
int HitNavItem(const NavGeometry& geo, const std::vector<NavItem>& items, float x, float y);
// 推进一个导航项的 hover 动画（dt 秒），返回是否仍在动画中
bool UpdateNavItemAnimation(NavItem& it, float dt);

} // namespace fz
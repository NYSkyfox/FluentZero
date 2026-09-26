#include "pch.h"
#include "UI/NavPane.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawNavPane(Renderer& r, const NavGeometry& g, const std::vector<NavItem>& items,
                 const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;

    float x = g.x, y = g.y, w = g.w, h = g.h;

    // 1) 磨砂面板底色（半透明，Acrylic 从透明处透出桌面模糊）
    D2D1_RECT_F pane{};
    pane.left = x; pane.top = y; pane.right = x + w; pane.bottom = y + h;
    rt->FillRectangle(&pane, r.MakeBrush(th.navPane).Get());

    // 2) 右侧 1px 分割线
    D2D1_POINT_2F l0{}, l1{};
    l0.x = x + w - 0.5f; l0.y = y;
    l1.x = x + w - 0.5f; l1.y = y + h;
    rt->DrawLine(l0, l1, r.MakeBrush(th.navBorder).Get(), 1.0f);

    // 3) 每个导航项
    const float padL = 8 * s;     // 条左右内缩
    const float iconX = x + 16 * s;
    const float textX = x + 48 * s;
    const float indW = 3 * s;     // 选中指示条宽
    for (int i = 0; i < (int)items.size(); i++) {
        const NavItem& it = items[i];
        float top = it.y, ih = it.h;
        if (ih <= 0) continue;
        float hT = EaseOut(it.hoverT);

        D2D1_RECT_F bar{};
        bar.left = x + padL; bar.top = top + 2 * s;
        bar.right = x + w - padL; bar.bottom = top + ih - 2 * s;

        // 3a) hover 灰条（alpha 随 hoverT 渐显）
        if (hT > 0.003f) {
            D2D1_COLOR_F hv = th.navHover;
            hv.a = hv.a * hT;
            rt->FillRectangle(&bar, r.MakeBrush(hv).Get());
            // 3b) Reveal 光带（选中项 hover 时叠加，近似 Win10 设置导航的 reveal）
            if (it.selected) {
                D2D1_COLOR_F sv = th.navSweep;
                sv.a = sv.a * hT;
                rt->FillRectangle(&bar, r.MakeBrush(sv).Get());
            }
        }

        // 3c) 选中指示条（左侧强调色竖条）
        if (it.selected) {
            D2D1_RECT_F ind{};
            ind.left = x; ind.top = top + 3 * s;
            ind.right = x + indW; ind.bottom = top + ih - 3 * s;
            D2D1_COLOR_F ac = FzCol(th.accent.r, th.accent.g, th.accent.b, 1);
            rt->FillRectangle(&ind, r.MakeBrush(ac).Get());
        }

        // 3d) 图标 + 文字
        D2D1_COLOR_F txt = th.text1;
        DWRITE_FONT_WEIGHT wt = it.selected ? (DWRITE_FONT_WEIGHT)600 : DWRITE_FONT_WEIGHT_NORMAL;
        float cy = top + ih * 0.5f;
        if (it.glyph) {
            wchar_t gl[2] = { (wchar_t)it.glyph, 0 };
            r.DrawText(gl, iconX, cy - 9 * s, 40 * s, L"Segoe MDL2 Assets", 16 * s,
                       DWRITE_FONT_WEIGHT_NORMAL, txt);
        }
        r.DrawText(it.text, textX, cy - 8 * s, w, L"Segoe UI", 13 * s, wt, txt);
    }
}

int HitNavItem(const NavGeometry& g, const std::vector<NavItem>& items, float x, float y) {
    if (x < g.x || x > g.x + g.w || y < g.y || y > g.y + g.h) return -1;
    for (int i = 0; i < (int)items.size(); i++) {
        const NavItem& it = items[i];
        if (y >= it.y && y <= it.y + it.h) return i;
    }
    return -1;
}

bool UpdateNavItemAnimation(NavItem& it, float dt) {
    it.hoverT = Clamp01(it.hoverT + (it.hot ? dt / 0.15f : -dt / 0.15f));
    return it.hoverT > 0 && it.hoverT < 1;
}

} // namespace fz
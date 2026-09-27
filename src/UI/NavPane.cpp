#include "pch.h"
#include "UI/NavPane.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawNavPane(Renderer& r, const NavGeometry& g, const std::vector<NavItem>& items,
                 const NavState& st, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;

    float x = g.x, y = g.y, w = g.w, h = g.h;
    float eT = EaseOut(st.t);   // 展开度（文字/标题透明度 + 图标位置用）

    // 1) 侧边栏底色：浅色半透明（不是黑色遮罩）。
//    注：CI 的 WARP 软渲染下 DWM BlurBehind 不真正模糊，纯透明会直接透出桌面图标；
//    加回高不透明浅底（本地真 Windows 上仍是半透明磨砂观感，CI 上挡住穿透）
    D2D1_RECT_F pane{};
    pane.left = x; pane.top = y; pane.right = x + w; pane.bottom = y + h;
    rt->FillRectangle(&pane, r.MakeBrush(th.navPane).Get());

    // 2) 右侧 1px 分割线
    D2D1_POINT_2F l0{}, l1{};
    l0.x = x + w - 0.5f; l0.y = y;
    l1.x = x + w - 0.5f; l1.y = y + h;
    rt->DrawLine(l0, l1, r.MakeBrush(th.navBorder).Get(), 1.0f);

    // 2b) 侧边栏顶部只留汉堡按钮（应用标题在窗口原生标题栏，侧边栏内不放标题，
//     避免与导航图标挤占基线——2018 规范）
    // 2c) 折叠/展开按钮（2018 NavigationView 规范：48x48 命中区，hover 画 40px
    //     圆角灰底，图标 20px GlobalNavButton，左边界 = NAV_PADDING_LEFT 16px，
    //     与下方导航图标严格共享同一垂直基准线）
    {
        const float hit = 48 * s;                 // 命中热区
        const float bs = 40 * s;                  // hover 圆角底视觉尺寸
        float bx = x + (hit - bs) * 0.5f, by = 8 * s + (hit - bs) * 0.5f;
        D2D1_ROUNDED_RECT bb = FzRR(bx, by, bx + bs, by + bs, 4 * s);
        if (st.btnHot) {
            D2D1_COLOR_F hv = th.navHover;
            rt->FillRoundedRectangle(&bb, r.MakeBrush(hv).Get());
        }
        wchar_t gl[2] = { 0xE700, 0 };   // GlobalNavButton（☰ 三条杠）
        r.DrawText(gl, x + 16 * s, by + bs * 0.5f - 10 * s, 24 * s, L"Segoe MDL2 Assets",
                   20 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text1);
    }

    // 3) 每个导航项
    const float padL = 8 * s;     // hover/选中背景条左右内缩
    const float iconX = x + 16 * s;   // 图标左边界 = NAV_PADDING_LEFT（汉堡/导航图标共享基准线）
    const float textX = x + 52 * s;   // 文字左边界 = 16 + 20(图标) + 16(间距)
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

        // 3c) 选中态（2018 NavigationView）：常驻浅灰圆角背景（左右内缩 8px）
        //     + 离左缘 8px 的 3×16px 圆角强调色短条（与图标垂直居中）
        if (it.selected) {
            D2D1_ROUNDED_RECT sel = FzRR(x + padL, top + 2 * s, x + w - padL,
                                         top + ih - 2 * s, 4 * s);
            D2D1_COLOR_F sv = th.navHover;   // 浅灰 rgba(0,0,0,0.06)
            rt->FillRoundedRectangle(&sel, r.MakeBrush(sv).Get());

            const float indH = 16 * s;
            D2D1_ROUNDED_RECT ind = FzRR(x + 8 * s, top + (ih - indH) * 0.5f,
                                         x + 8 * s + indW, top + (ih - indH) * 0.5f + indH,
                                         indW * 0.5f);
            D2D1_COLOR_F ac = FzCol(th.accent.r, th.accent.g, th.accent.b, 1);
            rt->FillRoundedRectangle(&ind, r.MakeBrush(ac).Get());
        }

        // 3d) 图标 + 文字
        D2D1_COLOR_F txt = th.text1;
        DWRITE_FONT_WEIGHT wt = it.selected ? (DWRITE_FONT_WEIGHT)600 : DWRITE_FONT_WEIGHT_NORMAL;
        float cy = top + ih * 0.5f;
        if (it.glyph) {
            wchar_t gl[2] = { (wchar_t)it.glyph, 0 };
            r.DrawText(gl, iconX, cy - 10 * s, 40 * s, L"Segoe MDL2 Assets", 20 * s,
                       DWRITE_FONT_WEIGHT_NORMAL, txt);
        }
        // 导航文字 + 当前状态（Selected / Hover / Idle）；折叠时随展开度淡出
        if (eT > 0.02f) {
            std::wstring nstate = it.selected ? L"Selected" : (it.hot ? L"Hover" : L"Idle");
            D2D1_COLOR_F ttxt = txt;
            ttxt.a *= eT;
r.DrawText(it.text + L" : " + nstate, textX, cy - 8 * s,
                        FzMx(20 * s, w - 66 * s), L"Segoe UI", 13 * s, wt, ttxt);
        }
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

bool HitNavPaneButton(const NavGeometry& g, float x, float y, float s) {
    const float hit = 48 * s;   // 48x48 命中区（与绘制位置一致：左上角）
    return x >= g.x && x <= g.x + hit && y >= 8 * s && y <= 8 * s + hit;
}

bool UpdateNavState(NavState& st, float dt) {
    float target = st.expanded ? 1.0f : 0.0f;
    float step = dt / 0.20f;   // 200ms 线性过渡（与 Reveal 的 EaseOut 视觉接近）
    if (st.t < target) st.t = Fzmn(target, st.t + step);
    else st.t = FzMx(target, st.t - step);
    return st.t != target;
}

} // namespace fz
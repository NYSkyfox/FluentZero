#include "pch.h"
#include "UI/RadioButton.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawRadioButton(Renderer& r, const RadioButton& rb, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;
    float hT = EaseOut(rb.hoverT);
    float d = 18 * s, rad = d * 0.5f;
    float cx = rb.x + rad, cy = rb.y + (rb.h - d) * 0.5f + rad;
    D2D1_ELLIPSE ell{};
    ell.point.x = cx; ell.point.y = cy; ell.radiusX = rad; ell.radiusY = rad;

    // 外圈（hover 时向 accent 过渡）
    D2D1_COLOR_F ring = Lerp(th.btnBorder, th.accent, 0.6f * hT);
    rt->DrawEllipse(&ell, r.MakeBrush(ring).Get(), (rb.selected ? 2.0f : 1.2f) * s);

    // 未选中 + hover：内环提示
    if (hT > 0.02f && !rb.selected) {
        D2D1_ELLIPSE hi = ell; hi.radiusX = rad - 3 * s; hi.radiusY = rad - 3 * s;
        D2D1_COLOR_F hc = th.navHover; hc.a *= hT;
        rt->DrawEllipse(&hi, r.MakeBrush(hc).Get(), 1.5f * s);
    }

    // 选中：中心 accent 圆点
    if (rb.selected) {
        D2D1_ELLIPSE dot{};
        dot.point.x = cx; dot.point.y = cy; dot.radiusX = 5 * s; dot.radiusY = 5 * s;
        rt->FillEllipse(&dot, r.MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
    }

    r.DrawText(rb.label, rb.x + d + 8 * s, cy - 8 * s, rb.w,
               L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text1);
}

int HitRadioButton(const std::vector<RadioButton>& v, float x, float y) {
    for (int i = 0; i < (int)v.size(); i++) {
        const RadioButton& c = v[i];
        if (x >= c.x && x <= c.x + c.w && y >= c.y && y <= c.y + c.h) return i;
    }
    return -1;
}

bool UpdateRadioButton(RadioButton& rb, float dt) {
    rb.hoverT = Clamp01(rb.hoverT + (rb.hot ? dt / 0.15f : -dt / 0.15f));
    return rb.hoverT > 0 && rb.hoverT < 1;
}

} // namespace fz
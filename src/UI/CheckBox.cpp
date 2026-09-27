#include "pch.h"
#include "UI/CheckBox.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawCheckBox(Renderer& r, const CheckBox& cb, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;
    float hT = EaseOut(cb.hoverT);
    float box = 20 * s, r2 = 3.5f * s;   // 20x20（Fluent 规范，原 18 偏小）
    float bx = cb.x, by = cb.y + (cb.h - box) * 0.5f;
    D2D1_ROUNDED_RECT rr = FzRR(bx, by, bx + box, by + box, r2);

    if (cb.checked) {
        float cT = EaseOut(cb.checkT);
        // 底色 + accent 按勾选进度淡入
        rt->FillRoundedRectangle(&rr, r.MakeBrush(th.btnFill).Get());
        D2D1_COLOR_F fill = FzCol(th.accent.r, th.accent.g, th.accent.b, cT);
        rt->FillRoundedRectangle(&rr, r.MakeBrush(fill).Get());
        // 白色勾（两段折线）
        if (cT > 0.05f) {
            D2D1_POINT_2F p1{}, p2{}, p3{};
            p1.x = bx + 4.5f * s;  p1.y = by + 10.5f * s;
            p2.x = bx + 9 * s;     p2.y = by + 14.5f * s;
            p3.x = bx + 15.5f * s; p3.y = by + 5.5f * s;
            D2D1_COLOR_F tick = FzCol(1, 1, 1, cT);
            auto b = r.MakeBrush(tick);
            rt->DrawLine(p1, p2, b.Get(), 1.8f * s);
            rt->DrawLine(p2, p3, b.Get(), 1.8f * s);
        }
    } else {
        rt->FillRoundedRectangle(&rr, r.MakeBrush(th.btnFill).Get());
        // 未选中边框：浅主题黑 45% / 深主题白 60%（原 btnBorder 10% 太浅）
        D2D1_COLOR_F unSel = th.light ? FzCol(0, 0, 0, 0.45f) : FzCol(1, 1, 1, 0.60f);
        D2D1_COLOR_F border = Lerp(unSel, th.accent, 0.5f * hT);
        rt->DrawRoundedRectangle(&rr, r.MakeBrush(border).Get(), (hT > 0 ? 1.5f : 1.0f) * s);
    }

    // 标签 + 实时状态（Checked/Unchecked）
    r.DrawText(cb.label + L" : " + (cb.checked ? L"Checked" : L"Unchecked"),
               cb.x + box + 8 * s, by + box * 0.5f - 8 * s, cb.w,
               L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text1);
}

int HitCheckBox(const std::vector<CheckBox>& v, float x, float y) {
    for (int i = 0; i < (int)v.size(); i++) {
        const CheckBox& c = v[i];
        if (x >= c.x && x <= c.x + c.w && y >= c.y && y <= c.y + c.h) return i;
    }
    return -1;
}

bool UpdateCheckBox(CheckBox& cb, float dt) {
    cb.hoverT = Clamp01(cb.hoverT + (cb.hot ? dt / 0.15f : -dt / 0.15f));
    cb.checkT = Clamp01(cb.checkT + (cb.checked ? dt / 0.12f : -dt / 0.12f));
    return (cb.hoverT > 0 && cb.hoverT < 1) || (cb.checkT > 0 && cb.checkT < 1);
}

} // namespace fz
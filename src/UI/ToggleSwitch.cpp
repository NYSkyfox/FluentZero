#include "pch.h"
#include "UI/ToggleSwitch.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawToggleSwitch(Renderer& r, const ToggleSwitch& ts, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;
    float hT = EaseOut(ts.hoverT);
    float tT = EaseOut(ts.toggleT);

    const float trackW = 40 * s, trackH = 20 * s;
    float tx = ts.x, ty = ts.y + (ts.h - trackH) * 0.5f;
    float knobD = trackH - 6 * s, knobR = knobD * 0.5f;
    // 圆钮中心：关=左，开=右，按 toggleT 滑动
    float knobX = tx + knobR + tT * (trackW - knobD);
    float knobY = ty + trackH * 0.5f;

    // 轨道：关=白底黑边，开=accent 填充（按 tT 过渡）
    D2D1_ROUNDED_RECT track = FzRR(tx, ty, tx + trackW, ty + trackH, trackH * 0.5f);
    D2D1_COLOR_F off = th.btnFill, on = FzCol(th.accent.r, th.accent.g, th.accent.b, 1);
    if (tT > 0.003f) {
        rt->FillRoundedRectangle(&track, r.MakeBrush(off).Get());
        rt->FillRoundedRectangle(&track, r.MakeBrush(FzCol(on.r, on.g, on.b, tT)).Get());
    } else {
        rt->FillRoundedRectangle(&track, r.MakeBrush(off).Get());
        rt->DrawRoundedRectangle(&track, r.MakeBrush(Lerp(th.btnBorder, th.accent, 0.5f * hT)).Get(), 1.0f);
    }

    // 圆钮
    D2D1_ELLIPSE knob{};
    knob.center.x = knobX; knob.center.y = knobY;
    knob.radiusX = knobR; knob.radiusY = knobR;
    rt->FillEllipse(&knob, r.MakeBrush(FzCol(1, 1, 1, 1)).Get());
    rt->DrawEllipse(&knob, r.MakeBrush(th.text1).Get(), 1.0f);

    r.DrawText(ts.label, ts.x + trackW + 8 * s, knobY - 8 * s, ts.w,
               L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text1);
}

int HitToggleSwitch(const std::vector<ToggleSwitch>& v, float x, float y) {
    for (int i = 0; i < (int)v.size(); i++) {
        const ToggleSwitch& c = v[i];
        if (x >= c.x && x <= c.x + c.w && y >= c.y && y <= c.y + c.h) return i;
    }
    return -1;
}

bool UpdateToggleSwitch(ToggleSwitch& ts, float dt) {
    ts.hoverT = Clamp01(ts.hoverT + (ts.hot ? dt / 0.15f : -dt / 0.15f));
    ts.toggleT = Clamp01(ts.toggleT + (ts.on ? dt / 0.15f : -dt / 0.15f));
    return (ts.hoverT > 0 && ts.hoverT < 1) || (ts.toggleT > 0 && ts.toggleT < 1);
}

} // namespace fz
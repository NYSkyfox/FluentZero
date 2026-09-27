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
    const float inset = 2 * s;               // 圆钮到轨道左右内壁的间隙（原为 0，贴边，现增大一点点）
    // 圆钮直径 14px（关）→ 12px（开），Win10 规范开态微缩
    float knobD = trackH - 6 * s - 2 * s * tT, knobR = knobD * 0.5f;
    // 圆钮中心：关=左，开=右，按 toggleT 滑动（两端各留 inset 间隙，不再贴边）
    float knobX = tx + inset + knobR + tT * (trackW - 2 * inset - knobD);
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

    // 圆钮（ON 态纯白无黑边：描边随 tT 从主题色过渡到白色，还原 Win10 原版观感）
    D2D1_ELLIPSE knob{};
    knob.point.x = knobX; knob.point.y = knobY;
    knob.radiusX = knobR; knob.radiusY = knobR;
    rt->FillEllipse(&knob, r.MakeBrush(FzCol(1, 1, 1, 1)).Get());
    // 软投影：+1px 下偏移（浅色 15% 黑 / 深色 35% 黑），给白滑块"浮起"感
    D2D1_ELLIPSE shadow{};
    shadow.point.x = knobX; shadow.point.y = knobY + 1 * s;
    shadow.radiusX = knobR; shadow.radiusY = knobR;
    rt->FillEllipse(&shadow, r.MakeBrush(FzCol(0, 0, 0, th.light ? 0.15f : 0.35f)).Get());
    // 圆钮描边：关态深色边、开态中灰边（45%），随 tT 平滑过渡，
    // 保证白色滑块在浅灰/白底上始终有清晰轮廓
    D2D1_COLOR_F offEdge = th.text1;
    D2D1_COLOR_F onEdge = FzCol(th.text1.r, th.text1.g, th.text1.b, 0.45f);
    D2D1_COLOR_F knobEdge = Lerp(offEdge, onEdge, tT);
    rt->DrawEllipse(&knob, r.MakeBrush(knobEdge).Get(), 1.0f);

    // 标签 + 实时状态（On/Off）
    r.DrawText(ts.label + L" : " + (ts.on ? L"On" : L"Off"),
               ts.x + trackW + 8 * s, knobY - 8 * s, ts.w,
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
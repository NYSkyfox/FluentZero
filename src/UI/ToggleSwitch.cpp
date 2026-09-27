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
    // ===== WinUI 3（Windows 11 Fluent）ToggleSwitch 规范 =====
    const float inset = 3 * s;               // 滑块到轨道内壁间隙
    // 滑块：12px（关）→ 14px（开），Win11 开态放大（与 Win10 微缩相反）；纯白、无投影、无描边
    float knobD = trackH - 8 * s + 2 * s * tT, knobR = knobD * 0.5f;
    // 滑块中心：关=左，开=右，按 toggleT 滑动
    float knobX = tx + inset + knobR + tT * (trackW - 2 * inset - knobD);
    float knobY = ty + trackH * 0.5f;

    // 轨道：OFF = 白底（深色主题 #333）；ON = accent 实底（按 tT 过渡）
    D2D1_ROUNDED_RECT track = FzRR(tx, ty, tx + trackW, ty + trackH, trackH * 0.5f);
    D2D1_COLOR_F off = th.light ? FzCol(1, 1, 1, 1) : FzCol(0.2f, 0.2f, 0.2f, 1);
    D2D1_COLOR_F on  = FzCol(th.accent.r, th.accent.g, th.accent.b, 1);
    rt->FillRoundedRectangle(&track, r.MakeBrush(off).Get());
    if (tT > 0.003f)
        rt->FillRoundedRectangle(&track, r.MakeBrush(FzCol(on.r, on.g, on.b, tT)).Get());
    // 轨道描边（常显 1px）：OFF = 灰（hover 加深）；ON = 深一度的 accent
    D2D1_COLOR_F offEdge = th.light ? FzCol(0.54f, 0.54f, 0.54f, 1) : FzCol(0.75f, 0.75f, 0.75f, 1);
    offEdge = Brighten(offEdge, -0.10f * hT);              // hover 加深 10%
    D2D1_COLOR_F onEdge = Brighten(on, -0.12f);             // accent 压暗 12%
    D2D1_COLOR_F edge = Lerp(offEdge, onEdge, tT);
    rt->DrawRoundedRectangle(&track, r.MakeBrush(edge).Get(), 1.0f);

    // 滑块：纯白实心圆 + 1px 描边——OFF 灰边（浅 #8A8A8A / 深 #B3B3B3）保证白滑块
    // 在白色 OFF 轨道上可见；ON 态描边透明（accent 底上白滑块自明）。随 tT 过渡
    D2D1_ELLIPSE knob{};
    knob.point.x = knobX; knob.point.y = knobY;
    knob.radiusX = knobR; knob.radiusY = knobR;
    rt->FillEllipse(&knob, r.MakeBrush(FzCol(1, 1, 1, 1)).Get());
    D2D1_COLOR_F knobOffEdge = th.light ? FzCol(0.54f, 0.54f, 0.54f, 1) : FzCol(0.70f, 0.70f, 0.70f, 1);
    D2D1_COLOR_F knobEdge = Lerp(knobOffEdge,
        FzCol(knobOffEdge.r, knobOffEdge.g, knobOffEdge.b, 0), tT);
    if (knobEdge.a > 0.01f)
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
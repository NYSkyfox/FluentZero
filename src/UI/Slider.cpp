#include "pch.h"
#include "UI/Slider.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawSlider(Renderer& r, const Slider& s, const FluentTheme& th, float sc) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;
    (void)EaseOut(s.hoverT);

    const float labelH = 16 * sc;
    const float trackH = 4 * sc;
    float trackY = s.y + labelH + (s.h - labelH - trackH) * 0.5f;
    float v = Clamp01(s.value);

    // 标签 + 实时百分比
    int pct = (int)(Clamp01(s.value) * 100.0f + 0.5f);
    r.DrawText(s.label + L" : " + std::to_wstring(pct) + L"%",
               s.x, s.y, s.w, L"Segoe UI", 11 * sc,
               DWRITE_FONT_WEIGHT_NORMAL, th.text2);

    // 轨道（灰）
    D2D1_RECT_F track{};
    track.left = s.x; track.top = trackY;
    track.right = s.x + s.w; track.bottom = trackY + trackH;
    rt->FillRectangle(&track, r.MakeBrush(th.cardBorder).Get());

    // 已选段（accent）
    if (v > 0.001f) {
        D2D1_RECT_F sel{};
        sel.left = s.x; sel.top = trackY;
        sel.right = s.x + s.w * v; sel.bottom = trackY + trackH;
        rt->FillRectangle(&sel, r.MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
    }

    // thumb（Fluent 规范：accent 实心圆 16px + 白色内圈；拖拽时略大）
    // 中心点 = 轨道中线，垂直居中精确
    float thumbR = (s.dragging ? 9.5f : 8.0f) * sc;
    float txc = s.x + s.w * v, tyc = trackY + trackH * 0.5f;
    D2D1_ELLIPSE thumb{};
    thumb.point.x = txc; thumb.point.y = tyc;
    thumb.radiusX = thumbR; thumb.radiusY = thumbR;
    rt->FillEllipse(&thumb, r.MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
    D2D1_ELLIPSE dot{};
    dot.point.x = txc; dot.point.y = tyc;
    dot.radiusX = 3.5f * sc; dot.radiusY = 3.5f * sc;
    rt->FillEllipse(&dot, r.MakeBrush(FzCol(1, 1, 1, 1)).Get());
}

int HitSlider(const std::vector<Slider>& v, float x, float y) {
    for (int i = 0; i < (int)v.size(); i++) {
        const Slider& c = v[i];
        if (x >= c.x && x <= c.x + c.w && y >= c.y && y <= c.y + c.h) return i;
    }
    return -1;
}

bool UpdateSlider(Slider& s, float dt) {
    bool target = (s.hot || s.dragging);
    s.hoverT = Clamp01(s.hoverT + (target ? dt / 0.15f : -dt / 0.15f));
    return s.hoverT > 0 && s.hoverT < 1;
}

} // namespace fz
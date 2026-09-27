#include "pch.h"
#include <cmath>
#include "UI/ProgressRing.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawProgressRing(Renderer& r, const ProgressRing& pr, const FluentTheme& th, float sc) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;

    const float ringD = 22 * sc;
    const float half = ringD * 0.5f;
    const float cx = pr.x + half;
    const float cy = pr.y + (pr.h - ringD) * 0.5f + half;
    const float ringT = 2.5f * sc;          // 环厚
    const float rr = half - ringT * 0.5f;   // 弧线半径

    // 底环（整圈，灰色）
    D2D1_ELLIPSE full{};
    full.point.x = cx; full.point.y = cy;
    full.radiusX = rr; full.radiusY = rr;
    rt->DrawEllipse(&full, r.MakeBrush(th.cardBorder).Get(), ringT);

    // 前景弧（accent，从顶部 -90° 顺时针，折线逼近避免依赖 D2D1_ARC 结构体）
    float v = Clamp01(pr.value);
    if (v > 0.002f) {
        int n = FzMx(2, (int)(v * 72.0f));
        float span = v * 2.0f * (float)M_PI;
        D2D1_POINT_2F prev{};
        bool first = true;
        for (int i = 0; i <= n; i++) {
            float ang = -0.5f * (float)M_PI + span * (float)i / (float)n;
            D2D1_POINT_2F pt{};
            pt.x = cx + rr * cosf(ang);
            pt.y = cy + rr * sinf(ang);
            if (!first)
                rt->DrawLine(prev, pt, r.MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get(), ringT);
            prev = pt; first = false;
        }
        // 端点圆帽
        rt->FillEllipse(&D2D1_ELLIPSE{ { prev.x, prev.y }, ringT * 0.5f, ringT * 0.5f },
                        r.MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
    }

    // 标签（右侧）
    r.DrawText(pr.label, pr.x + ringD + 8 * sc, cy - 8 * sc, pr.w,
               L"Segoe UI", 13 * sc, DWRITE_FONT_WEIGHT_NORMAL, th.text1);
}

} // namespace fz
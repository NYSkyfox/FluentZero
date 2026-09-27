#include "pch.h"
#include "UI/ProgressBar.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawProgressBar(Renderer& r, const ProgressBar& pb, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;
    const float barH = 4 * s;
    // 轨道固定在 label 文本下方（原为垂直居中 → 与 label 同高重叠遮挡）
    float by = pb.y + 16 * s;

    // 标签（左）+ 百分比（右）。百分比用固定宽框左对齐，不用 Measure——
    // CI 的 WARP 软渲染下 Measure 可能返回异常值（Button 文字消失的同款根因）
    r.DrawText(pb.label, pb.x, pb.y, pb.w, L"Segoe UI", 11 * s,
               DWRITE_FONT_WEIGHT_NORMAL, th.text2);
    {
        int pct = (int)(pb.value * 100.0f + 0.5f);
        std::wstring pctStr = std::to_wstring(pct) + L"%";
        const float pctBoxW = 36 * s;
        r.DrawText(pctStr, pb.x + pb.w - pctBoxW, pb.y, pctBoxW, L"Segoe UI", 11 * s,
                   DWRITE_FONT_WEIGHT_NORMAL, th.text1);
    }
    // 轨道
    D2D1_RECT_F track{};
    track.left = pb.x; track.top = by; track.right = pb.x + pb.w; track.bottom = by + barH;
    D2D1_COLOR_F trackC = th.cardBorder;
    rt->FillRectangle(&track, r.MakeBrush(trackC).Get());
    // 填充（accent，宽按 value 裁剪）
    float v = Clamp01(pb.value);
    if (v > 0.001f) {
        D2D1_RECT_F fill{};
        fill.left = pb.x; fill.top = by;
        fill.right = pb.x + pb.w * v; fill.bottom = by + barH;
        rt->FillRectangle(&fill, r.MakeBrush(FzCol(th.accent.r, th.accent.g, th.accent.b, 1)).Get());
    }
}

int HitProgressBar(const std::vector<ProgressBar>& v, float x, float y) {
    for (int i = 0; i < (int)v.size(); i++) {
        const ProgressBar& c = v[i];
        if (x >= c.x && x <= c.x + c.w && y >= c.y && y <= c.y + c.h) return i;
    }
    return -1;
}

} // namespace fz
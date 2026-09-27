#include "pch.h"
#include "UI/RatingControl.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

static const wchar_t kStarFill = 0xE734;    // Segoe MDL2 StarFill
static const wchar_t kStarEmpty = 0xE735;   // Segoe MDL2 StarOutline
static float StarGap(float s) { return 22 * s; }
static float StarSize(float s) { return 18 * s; }
static float StarLabelH(float s) { return 16 * s; }

void DrawRatingControl(Renderer& r, const RatingControl& rc, const FluentTheme& th, float sc) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;
    // 标签
    r.DrawText(rc.label, rc.x, rc.y, rc.w, L"Segoe UI", 11 * sc,
               DWRITE_FONT_WEIGHT_NORMAL, th.text2);
    float sy = rc.y + StarLabelH(sc);
    int shown = (rc.hover > 0) ? rc.hover : rc.value;
    for (int k = 1; k <= 5; k++) {
        float sx = rc.x + (k - 1) * StarGap(sc);
        bool on = k <= shown;
        D2D1_COLOR_F c = on ? FzCol(th.accent.r, th.accent.g, th.accent.b, 1) : th.cardBorder;
        std::wstring glyph(1, on ? kStarFill : kStarEmpty);
        r.DrawText(glyph, sx, sy, StarSize(sc), L"Segoe MDL2 Assets",
                   StarSize(sc), DWRITE_FONT_WEIGHT_NORMAL, c);
    }
}

int HitRatingControl(const std::vector<RatingControl>& v, float x, float y) {
    for (int i = 0; i < (int)v.size(); i++) {
        const RatingControl& c = v[i];
        if (x >= c.x && x <= c.x + c.w && y >= c.y && y <= c.y + c.h) return i;
    }
    return -1;
}

int RatingStarAt(const RatingControl& rc, float x, float sc) {
    for (int k = 1; k <= 5; k++) {
        float sx = rc.x + (k - 1) * StarGap(sc);
        if (x >= sx && x <= sx + StarSize(sc)) return k;
    }
    return 0;
}

} // namespace fz
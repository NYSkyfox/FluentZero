#include "pch.h"
#include "ColorUtils.h"
#include "MathUtils.h"

namespace fz {

D2D1_COLOR_F Premul(D2D1_COLOR_F c) {
    D2D1_COLOR_F o = c;
    o.r *= c.a; o.g *= c.a; o.b *= c.a;
    return o;
}

D2D1_COLOR_F Brighten(D2D1_COLOR_F c, float amt) {
    D2D1_COLOR_F o = c;
    o.r = Fzmn(1.0f, o.r * (1 + amt));
    o.g = Fzmn(1.0f, o.g * (1 + amt));
    o.b = Fzmn(1.0f, o.b * (1 + amt));
    return o;
}

D2D1_COLOR_F FzCol(float r, float g, float b, float a) {
    D2D1_COLOR_F c{};
    c.r = r; c.g = g; c.b = b; c.a = a;
    return c;
}

std::wstring HexOf(D2D1_COLOR_F c) {
    wchar_t buf[8];
    swprintf_s(buf, L"#%02X%02X%02X",
               (int)(c.r * 255 + 0.5f), (int)(c.g * 255 + 0.5f), (int)(c.b * 255 + 0.5f));
    return buf;
}

D2D1_COLOR_F Lerp(D2D1_COLOR_F a, D2D1_COLOR_F b, float t) {
    t = Clamp01(t);
    D2D1_COLOR_F c{};
    c.r = a.r + (b.r - a.r) * t;
    c.g = a.g + (b.g - a.g) * t;
    c.b = a.b + (b.b - a.b) * t;
    c.a = a.a + (b.a - a.a) * t;
    return c;
}

D2D1_ROUNDED_RECT FzRR(float l, float t, float r, float b, float rad) {
    D2D1_ROUNDED_RECT rc{};
    rc.rect.left = l; rc.rect.top = t;
    rc.rect.right = r; rc.rect.bottom = b;
    rc.radiusX = rad; rc.radiusY = rad;
    return rc;
}

} // namespace fz
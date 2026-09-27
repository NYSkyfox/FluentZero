#include "pch.h"
#include "Button.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawButton(Renderer& r, const Button& b, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;

    float h = EaseOut(b.hoverT), p = EaseOut(b.pressT), rv = EaseOut(b.revealT);

    D2D1_COLOR_F fill = b.primary ? th.accent : th.btnFill;
    fill.a = 1;
    fill = Brighten(fill, 0.04f * h);   // hover 提亮 4%
    fill = Brighten(fill, -0.08f * p);  // pressed 压暗 8%
    D2D1_COLOR_F txt = b.primary ? th.textOnAccent : th.btnText;

    float r3 = 3 * s;
    // 填充
    rt->FillRoundedRectangle(
        FzRR(b.x, b.y, b.x + b.w, b.y + b.h, r3),
        r.MakeBrush(fill).Get());
    // 边框（Win10 普通按钮有 1px 描边）
    if (!b.primary) {
        rt->DrawRoundedRectangle(
            FzRR(b.x + 0.5f, b.y + 0.5f, b.x + b.w - 0.5f, b.y + b.h - 0.5f, r3),
            r.MakeBrush(th.btnBorder).Get(), 1);
    }

    // Reveal 描边：整圈描边，alpha 随 revealT 渐入（Win10 Reveal 观感近似）
    if (rv > 0.003f) {
        D2D1_COLOR_F rc_ = th.reveal;
        rc_.a = rc_.a * rv;
        rt->DrawRoundedRectangle(
            FzRR(b.x + 0.5f, b.y + 0.5f, b.x + b.w - 0.5f, b.y + b.h - 0.5f, r3),
            r.MakeBrush(rc_).Get(), 1.5f * s);
    }

    // 文字：固定左对齐 + 当前状态（不用 Measure 居中——CI 软渲染下 Measure 可能返回异常值）
    std::wstring state = b.pressed ? L"Pressed" : (b.hot ? L"Hover" : L"Normal");
    r.DrawText(b.text + L" : " + state, b.x + 12 * s, b.y + b.h * 0.5f - 8 * s, b.w,
               L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, txt);
}

int HitButton(const std::vector<Button>& buttons, float x, float y) {
    for (int i = 0; i < (int)buttons.size(); i++) {
        const Button& b = buttons[i];
        if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) return i;
    }
    return -1;
}

bool UpdateButtonAnimation(Button& b, float dt) {
    b.hoverT  = Clamp01(b.hoverT + (b.hot ? dt / 0.15f : -dt / 0.15f));
    b.pressT  = Clamp01(b.pressT + (b.pressed ? dt / 0.08f : -dt / 0.12f));
    b.revealT = Clamp01(b.revealT + (b.hot ? dt / 0.15f : -dt / 0.15f));
    return (b.hoverT > 0 && b.hoverT < 1) || (b.pressT > 0 && b.pressT < 1) ||
           (b.revealT > 0 && b.revealT < 1);
}

} // namespace fz
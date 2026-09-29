#include "pch.h"
#include "pages/HomePage.h"
#include "UI/Button.h"
#include "UI/Card.h"
#include "Core/App.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"

namespace fz {

// 构造期填充控件（每种仅展示其样式/状态数）
HomePage::HomePage() {
    // Button 三种样式 + 禁用状态各一次
    buttons.push_back({ L"Button",  0, false, false, false });
    buttons.push_back({ L"Primary", 0, true,  false, false });
    buttons.push_back({ L"Subtle",  0, false, true,  false });
    buttons.push_back({ L"Disabled",0, false, false, true  });
    // CheckBox 两态：未勾选 / 已勾选
    checkboxes.push_back({ L"Animations", false, 0, 0, 0, 0, 0, 0, 0, false, false });
    checkboxes.push_back({ L"Sounds",     true,  0, 0, 0, 0, 0, 0, 0, false, false });
    // RadioButton 两态：选中 / 未选中
    radios.push_back({ L"Light", true,  0, 0, 0, 0, 0, false, false });
    radios.push_back({ L"Dark",  false,  0, 0, 0, 0, 0, false, false });
    // ToggleSwitch 三态：On / Off / Disabled
    toggles.push_back({ L"Notifications",   true,  0, 0, 0, 0, 0, 0, false, false });
    toggles.push_back({ L"Do not disturb",  false,  0, 0, 0, 0, 0, 0, false, false });
    toggles.push_back({ L"Airplane mode",   false,  0, 0, 0, 0, 0, 0, false, false, true });
    // ProgressBar / ProgressRing / Slider / Rating：各单一样式
    progressBars.push_back({ L"Syncing...", 0.6f, 0, 0, 0, 0, true });
    progressRings.push_back({ L"Loading...", 0.4f, 0, 0, 0, 0, true });
    sliders.push_back({ L"Brightness", 0.70f, 0, 0, 0, 0, 0, false, false });
    ratings.push_back({ L"How do you rate this?", 4, 0, 0, 0, 0, 0 });

    // Card 控件示例
    cards.push_back({ L"News card", L"Hello, this is a Card.\nIt supports multi-line content.\n右侧Demo卡片演示用。", 0, 0, 0, 0, false });
    cards.push_back({ L"Profile", L"Name: NYSkyfox\nRole: Developer\nLocation: Windows", 0, 0, 0, 0, false });
}

void HomePage::Layout(App& app, const PageRegion& r) {
    float x = r.x, y = r.y, w = r.w, h = r.h;
    float s = r.s;

    // 两栏 + 分组标题（对齐 Win10 设置页）
    float m = 28 * s;
    contentX = x + m;
    float cw = w - 2 * m;
    float gapCol = 28 * s;
    colW = (cw - gapCol) * 0.5f;
    colLX = contentX;
    colRX = contentX + colW + gapCol;
    float y0 = y;

    // ===== 左栏：Buttons / Selection =====
    {
        float ly = y0;
        gL1Y = ly;  ly += 16 * s + 10 * s;
        for (int i = 0; i < (int)buttons.size(); i++) {
            Button& b = buttons[i];
            b.x = colLX; b.y = ly; b.h = 32 * s; b.w = colW;
            ly += 32 * s + (i < (int)buttons.size() - 1 ? 10 * s : 18 * s);
        }
        gL2Y = ly;  ly += 16 * s + 10 * s;
        for (int i = 0; i < (int)checkboxes.size(); i++) {
            CheckBox& c = checkboxes[i];
            c.x = colLX; c.y = ly; c.h = 28 * s;
            float tw = app.Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 20 * s + 8 * s + tw + 84 * s;
            ly += 28 * s + (i < (int)checkboxes.size() - 1 ? 8 * s : 12 * s);
        }
        for (int i = 0; i < (int)toggles.size(); i++) {
            ToggleSwitch& c = toggles[i];
            c.x = colLX; c.y = ly; c.h = 28 * s;
            float tw = app.Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 40 * s + 8 * s + tw + 48 * s;
            ly += 28 * s + (i < (int)toggles.size() - 1 ? 8 * s : 12 * s);
        }
        for (int i = 0; i < (int)radios.size(); i++) {
            RadioButton& c = radios[i];
            c.x = colLX; c.y = ly; c.h = 28 * s;
            float tw = app.Measure(c.label, L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL);
            c.w = 20 * s + 8 * s + tw + 84 * s;
            ly += 28 * s + (i < (int)radios.size() - 1 ? 6 * s : 0);
        }
    }

    // ===== 右栏：Progress / Sliders / Rating / Cards =====
    {
        float ry = y0;
        gR1Y = ry;  ry += 16 * s + 10 * s;
        ProgressBar& p = progressBars[0];
        p.x = colRX; p.y = ry; p.h = 26 * s; p.w = colW;
        ry += 26 * s + 12 * s;
        for (int i = 0; i < (int)progressRings.size(); i++) {
            ProgressRing& rg = progressRings[i];
            rg.x = colRX; rg.y = ry; rg.h = 22 * s; rg.w = colW;
        }
        ry += 22 * s + 16 * s;
        gR2Y = ry;  ry += 16 * s + 10 * s;
        for (int i = 0; i < (int)sliders.size(); i++) {
            Slider& c = sliders[i];
            c.x = colRX; c.y = ry; c.h = 30 * s; c.w = colW;
            ry += 30 * s + (i < (int)sliders.size() - 1 ? 12 * s : 16 * s);
        }
        gR3Y = ry;  ry += 16 * s + 10 * s;
        {
            float ratingW = FzMx(20 * s * 5 + 10 * s, 160 * s);
            for (int i = 0; i < (int)ratings.size(); i++) {
                RatingControl& c = ratings[i];
                c.x = colRX; c.y = ry; c.h = 30 * s; c.w = ratingW;
                ry += 30 * s + (i < (int)ratings.size() - 1 ? 12 * s : 16 * s);
            }
        }
        gR4Y = ry;  ry += 16 * s + 10 * s;
        // Profile卡
        gR5Y = ry;  ry += 16 * s + 10 * s;
        cards[0].x = colRX; cards[0].y = ry; cards[0].w = colW;
        cards[0].h = 80 * s;
        ry += cards[0].h + 20 * s;

        // Account卡
        cards[1].x = colRX; cards[1].y = ry; cards[1].w = colW;
        cards[1].h = 80 * s;
    }
}

void HomePage::Draw(App& app) {
    const FluentTheme& th = app.th;
    float s = app.dpiScale;
    ID2D1HwndRenderTarget* rt = app.rt.Get();
    if (!rt) return;

    // 固定标题
    app.DrawText(L"BUTTONS",   colLX, gL1Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    app.DrawText(L"SELECTION", colLX, gL2Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    app.DrawText(L"PROGRESS",  colRX, gR1Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    app.DrawText(L"SLIDERS",   colRX, gR2Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    app.DrawText(L"RATING",    colRX, gR3Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);
    app.DrawText(L"PROFILE",   colRX, gR5Y, colW, L"Segoe UI", 11 * s, (DWRITE_FONT_WEIGHT)600, th.text2);

    // 左栏控件
    for (auto& b : buttons)   DrawButton(app, b, th, s);
    for (auto& c : checkboxes) DrawCheckBox(app, c, th, s);
    for (auto& t : toggles)   DrawToggleSwitch(app, t, th, s);
    for (auto& rb : radios)   DrawRadioButton(app, rb, th, s);
    // 右栏控件
    for (auto& p : progressBars)  DrawProgressBar(app, p, th, s);
    for (auto& rg : progressRings) DrawProgressRing(app, rg, th, s);
    for (auto& sl : sliders)      DrawSlider(app, sl, th, s);
    for (auto& rtg : ratings)     DrawRatingControl(app, rtg, th, s);

    // Card 控件
    for (int i = 0; i < (int)cards.size(); i++) DrawCardWidget(app, cards[i], th, s);
}

void HomePage::OnMove(App& app, float x, float y) {
    float s = app.dpiScale;
    int bh = HitButton(buttons, x, y);
    for (int i = 0; i < (int)buttons.size(); i++) buttons[i].hot = (i == bh);
    int ch = HitCheckBox(checkboxes, x, y);
    for (int i = 0; i < (int)checkboxes.size(); i++) checkboxes[i].hot = (i == ch);
    int th_ = HitToggleSwitch(toggles, x, y);
    for (int i = 0; i < (int)toggles.size(); i++) toggles[i].hot = (i == th_);
    int rh = HitRadioButton(radios, x, y);
    for (int i = 0; i < (int)radios.size(); i++) radios[i].hot = (i == rh);
    int sh = HitSlider(sliders, x, y);
    for (int i = 0; i < (int)sliders.size(); i++) sliders[i].hot = (i == sh);
    if (sliderDragIndex >= 0 && sliderDragIndex < (int)sliders.size()) {
        Slider& d = sliders[sliderDragIndex];
        if (d.w > 1.0f) d.value = FzMx(0.0f, Fzmn(1.0f, (x - d.x) / d.w));
    }
    for (auto& rtg : ratings)
        rtg.hover = (y >= rtg.y && y <= rtg.y + rtg.h) ? RatingStarAt(rtg, x, s) : 0;
    app.MarkDirty();
}

void HomePage::OnLButtonDown(App& app, float x, float y) {
    int hit = HitButton(buttons, x, y);
    if (hit >= 0) { buttons[hit].pressed = true; SetCapture(app.hwnd); app.MarkDirty(); return; }
    int sh = HitSlider(sliders, x, y);
    if (sh >= 0 && sliders[sh].w > 1.0f) {
        sliderDragIndex = sh;
        sliders[sh].dragging = true;
        sliders[sh].value = FzMx(0.0f, Fzmn(1.0f, (x - sliders[sh].x) / sliders[sh].w));
        SetCapture(app.hwnd);
        app.MarkDirty();
    }
}

void HomePage::OnLButtonUp(App& app, float x, float y) {
    if (GetCapture() == app.hwnd) ReleaseCapture();
    int hit = HitButton(buttons, x, y);
    for (int i = 0; i < (int)buttons.size(); i++) {
        Button& b = buttons[i];
        if (b.pressed) {
            b.pressed = false;
            if (i == hit && b.primary) { primaryClicks++; }
        }
    }
    int ch = HitCheckBox(checkboxes, x, y);
    if (ch >= 0) { checkboxes[ch].checked = !checkboxes[ch].checked; }
    int th_ = HitToggleSwitch(toggles, x, y);
    if (th_ >= 0) { toggles[th_].on = !toggles[th_].on; }
    int rh = HitRadioButton(radios, x, y);
    if (rh >= 0 && rh != radioSelected) {
        for (int i = 0; i < (int)radios.size(); i++) radios[i].selected = (i == rh);
        radioSelected = rh;
    }
    if (sliderDragIndex >= 0 && sliderDragIndex < (int)sliders.size()) {
        sliders[sliderDragIndex].dragging = false;
        sliderDragIndex = -1;
    }
    int rhg = HitRatingControl(ratings, x, y);
    if (rhg >= 0) {
        int star = RatingStarAt(ratings[rhg], x, app.dpiScale);
        if (star >= 1) ratings[rhg].value = star;
    }
    app.MarkDirty();
}

void HomePage::OnLeave(App& app) {
    for (auto& b : buttons) b.hot = false;
    for (auto& c : checkboxes) c.hot = false;
    for (auto& t : toggles) t.hot = false;
    for (auto& rb : radios) rb.hot = false;
    for (auto& sl : sliders) sl.hot = false;
    for (auto& rtg : ratings) rtg.hover = 0;
    app.MarkDirty();
}

bool HomePage::Update(App& app, float dt) {
    (void)app;
    bool anim = false;
    for (auto& b : buttons)   if (UpdateButtonAnimation(b, dt))   anim = true;
    for (auto& c : checkboxes) if (UpdateCheckBox(c, dt))          anim = true;
    for (auto& t : toggles)   if (UpdateToggleSwitch(t, dt))       anim = true;
    for (auto& rb : radios)   if (UpdateRadioButton(rb, dt))       anim = true;
    for (auto& p : progressBars) {
        if (p.active) { p.value += dt * 0.35f; if (p.value > 1.0f) p.value = 0.0f; anim = true; }
    }
    for (auto& rg : progressRings) {
        if (rg.active) { rg.value += dt * 0.30f; if (rg.value > 1.0f) rg.value = 0.0f; anim = true; }
    }
    for (auto& sl : sliders) if (UpdateSlider(sl, dt)) anim = true;
    return anim;
}

void HomePage::OnResize(App& app) {
    (void)app;
}

void HomePage::OnThemeChanged(App& app) {
    (void)app;
}

} // namespace fz
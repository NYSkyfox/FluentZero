#include "pch.h"
#include "UI/Card.h"
#include "Utils/MathUtils.h"
#include "Utils/ColorUtils.h"
#include "Theme/FluentTheme.h"
#include "Rendering/Renderer.h"

namespace fz {

void DrawCard(Renderer& r, const Card& c, const FluentTheme& th, float s) {
    ID2D1HwndRenderTarget* rt = r.rt.Get();
    if (!rt) return;

    const float rad = 4 * s;   // 圆角 4px

    // 背景填充
    D2D1_COLOR_F fill = th.card;
    rt->FillRoundedRectangle(FzRR(c.x, c.y, c.x + c.w, c.y + c.h, rad),
                             r.MakeBrush(fill).Get());

    // 1px 描边
    rt->DrawRoundedRectangle(
        FzRR(c.x + 0.5f, c.y + 0.5f, c.x + c.w - 0.5f, c.y + c.h - 0.5f, rad),
        r.MakeBrush(th.cardBorder).Get(), 1);

    // 可选标题
    if (!c.title.empty()) {
        r.DrawText(c.title, c.x + 16 * s, c.y + 8 * s, c.w - 32 * s,
                   L"Segoe UI", 17 * s, DWRITE_FONT_WEIGHT_MEDIUM, th.text1,
                   DWRITE_TEXT_ALIGNMENT_LEADING);
    }

    // 内容区域（支持换行）
    float contentY = c.y + (c.title.empty() ? 12 * s : 44 * s);
    float padX = 16 * s;
    float lineH = 20 * s;
    size_t pos = 0;
    size_t next = 0;
    int line = 0;
    while (pos < c.content.size()) {
        next = c.content.find(L'\n', pos);
        std::wstring lineText;
        if (next == std::wstring::npos) {
            lineText = c.content.substr(pos);
            pos = c.content.size();
        } else {
            lineText = c.content.substr(pos, next - pos);
            pos = next + 1;
        }
        float ty = contentY + line * lineH + (lineH - 16 * s) * 0.5f;  // 13px 字垂直居中
        r.DrawText(lineText, c.x + padX, ty, c.w - 2 * padX,
                   L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);
        line++;
    }
}

} // namespace fz
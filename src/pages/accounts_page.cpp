#include "pch.h"
#include "pages/AccountsPage.h"
#include "Core/App.h"

namespace fz {

void AccountsPage::Draw(App& app) {
    const FluentTheme& th = app.th;
    float s = app.dpiScale;
    app.DrawText(L"This is the Accounts page.", region.x + 28 * s, region.y + 12 * s, region.w - 56 * s,
                 L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);
}

} // namespace fz
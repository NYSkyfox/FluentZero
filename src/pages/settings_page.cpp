#include "pch.h"
#include "pages/SettingsPage.h"
#include "Core/App.h"

namespace fz {

void SettingsPage::Draw(App& app) {
    const FluentTheme& th = app.th;
    float s = app.dpiScale;
    // 占位内容：标题已由 App 画大标题，这里画一句说明
    app.DrawText(L"This is the Settings page.", region.x + 28 * s, region.y + 12 * s, region.w - 56 * s,
                 L"Segoe UI", 13 * s, DWRITE_FONT_WEIGHT_NORMAL, th.text2);
}

} // namespace fz
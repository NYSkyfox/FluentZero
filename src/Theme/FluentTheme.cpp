#include "pch.h"
#include "FluentTheme.h"
#include "Utils/ColorUtils.h"
#include "Platform/SystemSettings.h"

namespace fz {

FluentTheme FluentTheme::Create() {
    return Create(SystemPrefersLight());
}

FluentTheme FluentTheme::Create(bool light) {
    FluentTheme t;
    t.light  = light;
    t.accent = ReadAccent();
    if (t.light) {
        t.bg           = FzCol(0.957f, 0.957f, 0.957f, 0.55f);  // #F4F4F4 @55%（旧，弃用）
        t.contentBg    = FzCol(0.953f, 0.953f, 0.953f, 1.0f);  // #F3F3F3 实底（Win10 内容区不透出后方）
        t.card         = FzCol(1, 1, 1, 0.90f);
        t.cardBorder   = FzCol(0, 0, 0, 0.08f);
        t.text1        = FzCol(0, 0, 0, 0.96f);
        t.text2        = FzCol(0, 0, 0, 0.55f);
        t.textOnAccent = FzCol(1, 1, 1, 1);
        t.btnFill      = FzCol(0.98f, 0.98f, 0.98f, 0.95f);
        t.btnBorder    = FzCol(0, 0, 0, 0.10f);
        t.btnText      = FzCol(0, 0, 0, 0.96f);
        t.reveal       = FzCol(0, 0, 0, 0.30f);
        t.navPane      = FzCol(1, 1, 1, 0.35f);
        t.navBorder    = FzCol(0, 0, 0, 0.06f);
        t.navHover     = FzCol(0, 0, 0, 0.06f);
        t.navSweep     = FzCol(0, 0, 0, 0.05f);
    } else {
        t.bg           = FzCol(0.125f, 0.125f, 0.125f, 0.60f); // #202020（旧，弃用）
        t.contentBg    = FzCol(0.125f, 0.125f, 0.125f, 1.0f); // #202020 实底（Win10 内容区不透出后方）
        t.card         = FzCol(0.17f, 0.17f, 0.17f, 0.90f);
        t.cardBorder   = FzCol(1, 1, 1, 0.08f);
        t.text1        = FzCol(1, 1, 1, 0.96f);
        t.text2        = FzCol(1, 1, 1, 0.55f);
        t.textOnAccent = FzCol(1, 1, 1, 1);
        t.btnFill      = FzCol(0.20f, 0.20f, 0.20f, 0.95f);
        t.btnBorder    = FzCol(1, 1, 1, 0.10f);
        t.btnText      = FzCol(1, 1, 1, 0.96f);
        t.reveal       = FzCol(1, 1, 1, 0.30f);
        t.navPane      = FzCol(1, 1, 1, 0.08f);
        t.navBorder    = FzCol(1, 1, 1, 0.08f);
        t.navHover     = FzCol(1, 1, 1, 0.08f);
        t.navSweep     = FzCol(1, 1, 1, 0.12f);
    }
    return t;
}

} // namespace fz
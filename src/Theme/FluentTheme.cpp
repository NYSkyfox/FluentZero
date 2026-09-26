#include "pch.h"
#include "FluentTheme.h"
#include "Utils/ColorUtils.h"
#include "Platform/SystemSettings.h"

namespace fz {

FluentTheme FluentTheme::Create() {
    FluentTheme t;
    t.light  = SystemPrefersLight();
    t.accent = ReadAccent();
    if (t.light) {
        t.bg           = FzCol(0.957f, 0.957f, 0.957f, 0.55f);  // #F4F4F4 @55%
        t.card         = FzCol(1, 1, 1, 0.90f);
        t.cardBorder   = FzCol(0, 0, 0, 0.08f);
        t.text1        = FzCol(0, 0, 0, 0.96f);
        t.text2        = FzCol(0, 0, 0, 0.55f);
        t.textOnAccent = FzCol(1, 1, 1, 1);
        t.btnFill      = FzCol(0.98f, 0.98f, 0.98f, 0.95f);
        t.btnBorder    = FzCol(0, 0, 0, 0.10f);
        t.btnText      = FzCol(0, 0, 0, 0.96f);
        t.reveal       = FzCol(0, 0, 0, 0.30f);
    } else {
        t.bg           = FzCol(0.125f, 0.125f, 0.125f, 0.60f); // #202020
        t.card         = FzCol(0.17f, 0.17f, 0.17f, 0.90f);
        t.cardBorder   = FzCol(1, 1, 1, 0.08f);
        t.text1        = FzCol(1, 1, 1, 0.96f);
        t.text2        = FzCol(1, 1, 1, 0.55f);
        t.textOnAccent = FzCol(1, 1, 1, 1);
        t.btnFill      = FzCol(0.20f, 0.20f, 0.20f, 0.95f);
        t.btnBorder    = FzCol(1, 1, 1, 0.10f);
        t.btnText      = FzCol(1, 1, 1, 0.96f);
        t.reveal       = FzCol(1, 1, 1, 0.30f);
    }
    return t;
}

} // namespace fz
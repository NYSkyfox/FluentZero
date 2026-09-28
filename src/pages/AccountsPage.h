#pragma once

// 账户页（占位）：文件命名约定 accounts_page.h / accounts_page.cpp
#include "pages/Page.h"

namespace fz {

class AccountsPage : public Page {
public:
    std::wstring Title() const override { return L"Accounts"; }
    void Layout(App& app, const PageRegion& r) override {}
    void Draw(App& app) override;
    void OnMove(App& app, float x, float y) override {}
    void OnLButtonDown(App& app, float x, float y) override {}
    void OnLButtonUp(App& app, float x, float y) override {}
};

} // namespace fz
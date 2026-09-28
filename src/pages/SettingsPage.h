#pragma once

// 设置页（占位）：文件命名约定 settings_page.h / settings_page.cpp
#include "pages/Page.h"

namespace fz {

// 占位页：显示页面标题 + 提示文字，展示"一个导航项对应一个页面文件"的结构
class SettingsPage : public Page {
public:
    std::wstring Title() const override { return L"Settings"; }
    void Layout(App& app, const PageRegion& r) override {}
    void Draw(App& app) override;
    void OnMove(App& app, float x, float y) override {}
    void OnLButtonDown(App& app, float x, float y) override {}
    void OnLButtonUp(App& app, float x, float y) override {}
};

} // namespace fz
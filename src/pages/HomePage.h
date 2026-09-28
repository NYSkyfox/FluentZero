#pragma once

// 首页：Win10 Fluent 控件展示页（两栏布局，每种控件仅展示其样式/状态数）。
// 文件命名约定：home_page.h / home_page.cpp
// 依赖：Page（基类）+ 各 UI 控件
#include "pages/Page.h"
#include "UI/Button.h"
#include "UI/CheckBox.h"
#include "UI/RadioButton.h"
#include "UI/ToggleSwitch.h"
#include "UI/ProgressBar.h"
#include "UI/ProgressRing.h"
#include "UI/Slider.h"
#include "UI/RatingControl.h"
#include "UI/Card.h"

namespace fz {

class HomePage : public Page {
public:
    HomePage();   // 构造期填充控件（每种仅展示其样式/状态数）
    std::wstring Title() const override { return L"Home"; }

    void Layout(App& app, const PageRegion& r) override;
    void Draw(App& app) override;
    void OnMove(App& app, float x, float y) override;
    void OnLButtonDown(App& app, float x, float y) override;
    void OnLButtonUp(App& app, float x, float y) override;
    void OnLeave(App& app) override;
    bool Update(App& app, float dt) override;
    void OnResize(App& app) override;
    void OnThemeChanged(App& app) override;

    // 控件（每种仅展示其样式/状态数）
    std::vector<Button> buttons;          // 标准 / Primary / Subtle / Disabled
    std::vector<CheckBox> checkboxes;     // 未勾选 / 已勾选
    std::vector<RadioButton> radios;      // 选中 / 未选中
    std::vector<ToggleSwitch> toggles;    // On / Off / Disabled
    std::vector<ProgressBar> progressBars;// 单一样式
    std::vector<ProgressRing> progressRings; // 单一样式
    std::vector<Slider> sliders;          // 单一样式
    std::vector<RatingControl> ratings;   // 单一样式
    std::vector<Card> cards;             // 卡片示例

    // 供 App 读取（Info 卡的 "Primary clicks"）
    int primaryClicks = 0;
    int radioSelected = 0;   // RadioButton 组内互斥（默认选 0）

private:
    // Info 卡
    static const int kDetailRows = 6;
    std::wstring detailLabel[kDetailRows], detailValue[kDetailRows];
    void RebuildDetail(App& app, const std::wstring& pageTitle);
    void DrawCard(App& app);

    // 布局坐标
    float contentX = 0, colLX = 0, colRX = 0, colW = 0;
    float gL1Y = 0, gL2Y = 0, gR1Y = 0, gR2Y = 0, gR3Y = 0, gR4Y = 0;
    float cardX = 0, cardY = 0, cardW = 0, cardH = 0;
    int sliderDragIndex = -1;
};

} // namespace fz
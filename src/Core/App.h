#pragma once

// 应用编排层：窗口生命周期 / 布局 / 输入 / 消息循环 / 动画调度
// 依赖所有下层：Rendering、UI、Theme、Platform、Utils
#include "Rendering/Renderer.h"
#include "UI/Button.h"
#include "UI/NavPane.h"
#include "UI/CheckBox.h"
#include "UI/RadioButton.h"
#include "UI/ToggleSwitch.h"
#include "UI/ProgressBar.h"
#include "UI/ProgressRing.h"
#include "UI/Slider.h"
#include "UI/RatingControl.h"
#include "Theme/FluentTheme.h"

namespace fz {

class App : public Renderer {
public:
    HWND hwnd = nullptr;
    FluentTheme th;
    std::vector<Button> buttons;
    std::vector<NavItem> navItems;
    NavGeometry navGeo;
    int navSelected = 0;
    std::vector<CheckBox> checkboxes;
    std::vector<RadioButton> radios;
    int radioSelected = 0;
    std::vector<ToggleSwitch> toggles;
    std::vector<ProgressBar> progressBars;
    std::vector<ProgressRing> progressRings;
    std::vector<Slider> sliders;
    std::vector<RatingControl> ratings;
    float dpiScale = 1.0f;

    // 创建窗口 + 初始化渲染 + 填充按钮
    HRESULT Create();
    // 消息循环（含动画驱动）
    void Run();

private:
    // 状态
    float lastT = 0;
    bool needsDraw = true;
    bool animating = false;
    bool quit = false;
    int primaryClicks = 0;
    int sliderDragIndex = -1;    // 正在拖拽的 slider（-1 无）
    // Info 卡数据：三行（标签 + 值），DrawCard 双列固定 X 网格绘制
    static const int kDetailRows = 3;
    std::wstring detailLabel[kDetailRows], detailValue[kDetailRows];
    std::wstring pageTitle = L"Home";   // 右侧内容区大标题（跟随导航选中项）
    bool currentLight = true;            // 当前已应用的主题（用于检测切换）
    ULONGLONG lastThemePollMs = 0;       // 上次主题轮询时间（ms）

    // 布局坐标（供 onDraw 使用）
    float titleY = 0, subY = 0;
    float cardX = 0, cardY = 0, cardW = 0, cardH = 0;
    float contentX = 0;      // 右侧内容区起点（导航栏宽度之后）
    float colLX = 0, colRX = 0, colW = 0;        // 内容区两栏：左栏 X / 右栏 X / 栏宽
    float gL1Y = 0, gL2Y = 0;                    // 左栏分组标题 Y：Buttons / Selection
    float gR1Y = 0, gR2Y = 0, gR3Y = 0, gR4Y = 0; // 右栏分组标题 Y：Progress / Sliders / Rating / Info

    // 布局
    void Layout();
    void RebuildDetail();
    // 实时跟随系统深浅主题：检测变化则重建主题 + 重绘
    void CheckThemeChange();

    // 输入
    void OnMove(float x, float y);
    void OnLButtonDown(float x, float y);
    void OnLButtonUp(float x, float y);

    // 动画
    void Update(float dt);

    // 绘制（Renderer::onDraw 实现）
    void onDraw() override;
    void DrawCard();

    // Win32 消息
    static LRESULT CALLBACK WndProcStatic(HWND h, UINT m, WPARAM w, LPARAM l);
    LRESULT WndProc(HWND h, UINT m, WPARAM w, LPARAM l);
};

} // namespace fz
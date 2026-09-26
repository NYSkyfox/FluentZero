#pragma once

// 应用编排层：窗口生命周期 / 布局 / 输入 / 消息循环 / 动画调度
// 依赖所有下层：Rendering、UI、Theme、Platform、Utils
#include "Rendering/Renderer.h"
#include "UI/Button.h"
#include "UI/NavPane.h"
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
    std::wstring detailAccent, detailTheme, detailClicks;
    std::wstring pageTitle = L"Home";   // 右侧内容区大标题（跟随导航选中项）

    // 布局坐标（供 onDraw 使用）
    float titleY = 0, subY = 0;
    float cardX = 0, cardY = 0, cardW = 0, cardH = 0;
    float contentX = 0;     // 右侧内容区起点（导航栏宽度之后）

    // 布局
    void Layout();
    void RebuildDetail();

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
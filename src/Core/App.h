#pragma once

// 应用编排层（壳）：窗口生命周期 / 左侧导航栏 / 大标题·副标题 / 输入与动画调度。
// 页面内容（控件、布局、绘制、命中、动画）全部下放到 src/pages/ 下的 Page 子类，
// 一个页面对应一个文件（home_page / settings_page / accounts_page / network_page）。
// 依赖：Rendering、Theme、UI/NavPane、pages/Page
#include "Rendering/Renderer.h"
#include "UI/NavPane.h"
#include "pages/Page.h"
#include "Theme/FluentTheme.h"

namespace fz {

class App : public Renderer {
public:
    HWND hwnd = nullptr;
    FluentTheme th;

    // 导航栏（壳层职责：折叠/选中/切换页面）
    std::vector<NavItem> navItems;
    NavGeometry navGeo;
    NavState navState;
    int navSelected = 0;
    float dpiScale = 1.0f;

    // 页面：与 navItems 一一对应（同一顺序），切换导航即切换当前页
    std::vector<Page*> pages;

    // 创建窗口 + 初始化渲染 + 创建页面
    HRESULT Create();
    // 消息循环（含动画驱动）
    void Run();
    // 供页面标记"需要重绘"
    void MarkDirty() { needsDraw = true; }
    // 当前页面
    Page* CurrentPage() const { return pages.empty() ? nullptr : pages[navSelected]; }
    // 析构：释放所有页面
    ~App() { for (auto p : pages) delete p; }

private:
    // 状态
    float lastT = 0;
    bool needsDraw = true;
    bool animating = false;
    bool quit = false;
    bool currentLight = true;            // 当前已应用的主题（用于检测切换）
    ULONGLONG lastThemePollMs = 0;       // 上次主题轮询时间（ms）

    // 内容区矩形（导航栏之后）+ 大标题 / 副标题位置
    PageRegion contentRegion;
    float titleY = 0, subY = 0;
    float contentX = 0;                  // 大标题 X（导航栏宽度 + 内边距）

    // 布局
    void Layout();
    // 实时跟随系统深浅主题：检测变化则重建主题 + 通知页面 + 重绘
    void CheckThemeChange();

    // 输入
    void OnMove(float x, float y);
    void OnLButtonDown(float x, float y);
    void OnLButtonUp(float x, float y);

    // 动画
    void Update(float dt);

    // 绘制（Renderer::onDraw 实现）
    void onDraw() override;

    // Win32 消息
    static LRESULT CALLBACK WndProcStatic(HWND h, UINT m, WPARAM w, LPARAM l);
    LRESULT WndProc(HWND h, UINT m, WPARAM w, LPARAM l);
};

} // namespace fz
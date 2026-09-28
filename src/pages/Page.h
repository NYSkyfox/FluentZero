#pragma once

// 页面基类：一个页面对应一个文件（命名形如 xxx_page.cpp，如 home_page.cpp）。
// 职责划分：
//   App  —— 窗口生命周期 / 左侧导航栏 / 大标题·副标题 / 输入与动画调度
//   Page —— 当前页面的内容区：控件、布局、绘制、命中、动画
// 新增页面：建一个 Page 子类（xxx_page.h/.cpp）→ 在 App::Create() 里按导航顺序加入 pages 向量。
namespace fz {

class App;   // 前向声明（App 继承 Renderer，页面通过它访问 rt / MakeBrush / DrawText / 主题）

// 页面在内容区可用的矩形 + 缩放（由 App::Layout 计算后传入）
struct PageRegion {
    float x = 0, y = 0, w = 0, h = 0;
    float s = 1.0f;   // dpiScale
};

class Page {
public:
    virtual ~Page() = default;

    // 页面标题（App 绘制为右侧大标题；应与对应导航项文字一致）
    virtual std::wstring Title() const = 0;

    // 在给定内容区里布置本页面控件
    virtual void Layout(App& app, const PageRegion& r) = 0;
    // 绘制本页面内容（App 画完背景 / 导航 / 大标题后调用）
    virtual void Draw(App& app) = 0;

    // 输入（窗口客户区坐标；页面自行命中其控件）
    virtual void OnMove(App& app, float x, float y) = 0;
    virtual void OnLButtonDown(App& app, float x, float y) = 0;
    virtual void OnLButtonUp(App& app, float x, float y) = 0;
    // 指针离开窗口：清除本页面控件 hover（默认无）
    virtual void OnLeave(App& app) {}

    // 每帧动画推进；返回是否仍在动画中
    virtual bool Update(App& app, float dt) { return false; }
    // 窗口尺寸变化（用于刷新依赖客户区的信息，如尺寸）
    virtual void OnResize(App& app) {}
    // 系统深浅主题切换（用于刷新依赖主题的信息）
    virtual void OnThemeChanged(App& app) {}

protected:
    // 本页面当前内容区（App::Layout 每帧写入；Draw/命中据此定位）
    PageRegion region;
};

} // namespace fz

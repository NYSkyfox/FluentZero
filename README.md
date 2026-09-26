# FluentZero

**Windows 10 (2017–2018) 风格 Fluent Design，纯 Win32 + Direct2D 手搓。**

零第三方依赖、零 NuGet、零附带 DLL——一个单文件 `main.cpp` + 预编译头，
产物是**单个 exe**（静态链接 CRT，约几百 KB），目标机无需安装任何运行时。

> 对比：同样效果的 WinUI 3 自包含应用约 **24 MB（zip）/ 60 MB（解压）**。
> 那些字节全部花在了 XAML 框架本体上，没有一字节花在"画图"。

## 技术栈（全部系统自带）

| 能力 | 组件 |
|---|---|
| 窗口 / 消息 | Win32 (`user32`) |
| Acrylic 模糊 | DWM `DwmEnableBlurBehindWindow`（Win10 1607+ 经典实现） |
| 独立合成层 | DirectComposition |
| 2D 绘制 | Direct2D 1.1（flip swap chain + `ID2D1DeviceContext`） |
| 文本 / 图标 | DirectWrite（Segoe UI / Segoe UI Semibold / Segoe MDL2 Assets） |

## 演示的 Win10 Fluent 要素

1. **Acrylic 背景** — `BlurBehind` 模糊 + 半透明主题底色
   （浅色 `#F4F4F4`@55% / 深色 `#202020`@60%）
2. **系统强调色** — 读注册表
   `HKCU\Software\Microsoft\Windows\CurrentVersion\ThemeManager\AccentColor`
   （读不到时回退 Win10 默认蓝 `#0078D7`）
3. **深浅主题自动跟随** — `AppsUseLightTheme`
4. **Button（Reveal）** — hover 提亮 4% / pressed 压暗 8% / Reveal 描边
   渐入，150 ms ease-out cubic，对齐 WinUI Button 交互
5. **Segoe MDL2 Assets 图标** — Home / Settings / Refresh / Add

## 构建

- Visual Studio 2022（v143 toolset）+ Windows 10 SDK（任意版本）
- `msbuild FluentZero.sln /p:Configuration=Release /p:Platform=x64`
- 或直接在 VS 里打开 `FluentZero.sln`，Release | x64 生成

产物：`bin/x64/Release/FluentZero.exe`（无任何附带文件）

仓库已配置 **GitHub Actions**（手动触发）：
Actions 页面 → `build` → **Run workflow**，产物在 Artifacts 里下载。

## 代码结构（`main.cpp`，约 650 行）

```
fz::FluentTheme   调色板：浅/深主题 + 系统强调色（注册表读取）
fz::Button        按钮状态机：hot / pressed + hoverT / pressT / revealT 动画进度
fz::Renderer      渲染后端：DComp target + flip swap chain + D2D DeviceContext + DWrite
fz::App           窗口、布局、输入命中检测、动画更新、消息循环
```

渲染管线：

```
HWND（普通带标题栏窗口）
 └─ IDCompositionTarget（CreateTargetForHwnd）
     └─ IDCompositionSurface（CreateSurface 包住 swap chain）
         └─ DXGI flip swap chain（B8G8R8A8 预乘 alpha）
             └─ ID2D1DeviceContext（DrawTextLayout / FillRoundedRectangle / ...）
```

## 渲染架构说明

- 窗口使用普通 GDI 边框（保留原生标题栏/阴影/边框，这是 Win10 风格的一部分），
  客户区由 DirectComposition 接管渲染，因此可以输出**半透明像素**——
  半透明处 DWM 把窗口后方的模糊内容透出，形成 Acrylic。
- 空闲时 `WaitMessage()` 阻塞（零 CPU），仅在有动画时进入 16 ms 帧循环。

## 已知简化（demo 范围内有意为之）

- Reveal 描边采用"整圈 alpha 渐入"，不是 Win10 原版"从进入边扫一圈"
  （后者需要 D2D 自定义几何分段，留待完整版）
- 无 IME / 文本输入框（这正是手搓方案最大的坑，demo 有意绕开）
- 无 UIA 无障碍桥接（正式产品需要）
- 按钮布局为固定演示布局，不是通用布局引擎

## 路线图（MVP → 完整版，预估 2,000–3,500 行 / 2–3 周）

- [ ] 完整版 Reveal（沿边扫过）
- [ ] CheckBox / ToggleSwitch / RadioButton
- [ ] 焦点管理（Tab 导航 + 焦点描边）
- [ ] 滚动条 / 列表（ListView 雏形）
- [ ] 窗口圆角可选（Win10 无圆角，Win11 用 DWM 属性）
- [ ] 可选 UIA 桥接

## 许可

MIT
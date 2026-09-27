# FluentZero

**Windows 10 (2017–2018) 风格 Fluent Design，纯 Win32 + Direct2D 手搓。**

零第三方依赖、零 NuGet、零附带 DLL——分层多文件结构（Utils / Platform / Theme /
Rendering / UI / Core），产物是**单个 exe**（静态链接 CRT，约几百 KB），
目标机无需安装任何运行时。

> 对比：同样效果的 WinUI 3 自包含应用约 **24 MB（zip）/ 60 MB（解压）**。
> 那些字节全部花在了 XAML 框架本体上，没有一字节花在"画图"。

## 技术栈（全部系统自带）

| 能力 | 组件 |
|---|---|
| 窗口 / 消息 | Win32 (`user32`) |
| Acrylic 模糊 | DWM `DwmEnableBlurBehindWindow`（Win10 1607+ 经典实现） |
| 2D 绘制 | Direct2D 1.0 `HwndRenderTarget`（`WS_EX_NOREDIRECTIONBITMAP` 直连 DWM） |
| 文本 / 图标 | DirectWrite（Segoe UI / Segoe MDL2 Assets / Consolas） |

## 演示的 Win10 Fluent 要素

两栏"设置页"演示布局（左：Buttons / Selection，右：Progress / Sliders /
Rating / Info），覆盖 8 类核心控件 + 可折叠导航：

1. **Acrylic 背景** — `BlurBehind` 模糊，仅作用于左侧导航栏
   （近实底 `#F3F3F3`@96% / `#202020`@96%，CI 软渲染下挡住穿透）；
   内容区为实底 `#F3F3F3` / `#202020`
2. **系统强调色** — 读注册表
   `HKCU\Software\Microsoft\Windows\CurrentVersion\ThemeManager\AccentColor`
   （读不到时回退 Win10 默认蓝 `#0078D7`）
3. **深浅主题自动跟随** — `AppsUseLightTheme`
4. **Button（Reveal）** — hover 提亮 4% / pressed 压暗 8% / Reveal 描边
   渐入，150 ms ease-out cubic，对齐 WinUI Button 交互
5. **ToggleSwitch（WinUI 3 / Win11 配色）** — OFF 白轨道 + 灰滑块，
   ON accent 轨道 + 白滑块；12→14px 开态放大；标签实时 On/Off
6. **Segoe MDL2 Assets 图标** — Home / Settings / Accounts / Network
   + 折叠按钮 GlobalNavButton（三条杠）
7. **可折叠导航栏** — 左上角三条杠按钮，180↔48px 200 ms 动画，
   折叠后仅图标（文字淡出），内容区随之平移
8. **实时状态文本** — 所有组件标签追加当前状态
   （Button: Normal/Hover/Pressed、Checkbox: Checked/Unchecked、
   Slider: N%、Rating: N/5、Nav: Selected/Hover/Idle…）

## 构建

- Visual Studio 2022（v143 toolset）+ Windows 10 SDK（任意版本）
- `msbuild FluentZero.sln /p:Configuration=Release /p:Platform=x64`
- 或直接在 VS 里打开 `FluentZero.sln`，Release | x64 生成

产物：`bin/x64/Release/FluentZero.exe`（无任何附带文件）
仓库已配置 **GitHub Actions**（手动触发）：
Actions 页面 → `build` → **Run workflow**，产物在 Artifacts 里下载。

> **构建验证（2026-09-27，windows-2025 VS2026 Runner，SDK 10.0.26100）**：
> Release|x64 构建通过，产物 **FluentZero.exe = 209 KB**（单文件，零附带 DLL）。
> 静态链接 CRT（`/MT`），Win10 1607+ / Win11 直接双击运行。
> CI 同时产出全屏截图存档（命名 `FluentZero_<sha7>_<时间戳>.png`）。

## 代码结构（分层架构）

入口 `main.cpp` + 预编译头 `pch.{h,cpp}` 在项目根目录，6 个功能层放在 `src/`，
自底向上单向依赖（上层可引用下层，反之不行）：

```
├── main.cpp              入口 wWinMain（根目录）
├── pch.{h,cpp}           预编译头（根目录）
└── src/
    ├── Utils/            第 1 层 · 纯工具（无业务依赖）
    │   ├── MathUtils.h       FzMx/Fzmn/Clamp01/EaseOut（header-only）
    │   └── ColorUtils.{h,cpp} D2D1_COLOR_F：Premul/Brighten/FzCol/HexOf/FzRR
    ├── Platform/         第 2 层 · OS 抽象（只依赖 Win32）
    │   └── SystemSettings.{h,cpp} ReadAccent/SystemPrefersLight/GetEffectiveDpi
    ├── Theme/            第 3 层 · Fluent 主题（依赖 Utils + Platform）
    │   └── FluentTheme.{h,cpp}  浅/深调色板 + 系统强调色
    ├── Rendering/        第 4 层 · 渲染后端（D2D1 + DirectWrite）
    │   └── Renderer.{h,cpp}     HwndRenderTarget 基类 + 文本绘制工具
    ├── UI/               第 5 层 · UI 控件（依赖 Rendering + Theme + Utils）
    │   ├── Button.{h,cpp}        Reveal 按钮：状态 + 动画 + 绘制 + 命中
    │   ├── NavPane.{h,cpp}       侧边导航：磨砂面板 + hover 渐显 + Reveal 光带
    │   ├── CheckBox.{h,cpp}      复选框：accent 填充 + 白色勾
    │   ├── RadioButton.{h,cpp}   单选按钮：圆外圈 + 中心点（组内互斥）
    │   ├── ToggleSwitch.{h,cpp}  开关：轨道 + 滑动圆钮
    │   ├── ProgressBar.{h,cpp}   进度条：轨道 + accent 填充
    │   ├── ProgressRing.{h,cpp}  进度环：底环 + accent 弧（折线逼近）+ 端点圆帽
    │   ├── Slider.{h,cpp}        滑块：标签 + 轨道 + accent 已选段 + 可拖拽 thumb
    │   └── RatingControl.{h,cpp} 星级评分：5 颗星（MDL2 字形）+ hover 预览
    └── Core/             第 6 层 · 应用编排（依赖所有层）
        └── App.{h,cpp}          窗口 / 布局 / 输入 / 消息循环 / 动画调度
```

依赖方向（单向、无环）：

```
Core ──> UI ──> Rendering ──> Utils
 │         │          │
 └──> Theme <─────────┘
        │
        └──> Platform ──> (Win32)
```

渲染管线：

```
HWND（WS_EX_NOREDIRECTIONBITMAP，保留原生标题栏）
 └─ DwmEnableBlurBehindWindow（Acrylic：模糊窗口后方内容）
     └─ D2D1 HwndRenderTarget（B8G8R8A8 预乘 alpha）
         └─ DrawTextLayout / FillRoundedRectangle / ...
```

## 渲染架构说明

- 窗口保留原生 GDI 边框（标题栏/阴影/边框，这是 Win10 风格的一部分），
  客户区设 `WS_EX_NOREDIRECTIONBITMAP` 由 D2D1 `HwndRenderTarget` 直接渲染，
  因此可输出**半透明像素**——半透明处 DWM 把窗口后方的模糊内容透出，形成 Acrylic。
- 空闲时 `WaitMessage()` 阻塞（零 CPU），仅在有动画时进入 16 ms 帧循环。

## 已知简化（demo 范围内有意为之）

- Reveal 描边采用"整圈 alpha 渐入"，不是 Win10 原版"从进入边扫一圈"
  （后者需要 D2D 自定义几何分段，留待完整版）
- 无 IME / 文本输入框（这正是手搓方案最大的坑，demo 有意绕开）
- 无 UIA 无障碍桥接（正式产品需要）
- 按钮布局为固定演示布局，不是通用布局引擎

## 路线图（MVP → 完整版，预估 2,000–3,500 行 / 2–3 周）

### 控件完成度对照（对标 Windows 10 UWP 控件库）

> 说明：Fluent Design 本身是设计语言，不定义控件数量。下表对标 2018 年承载它的
> Windows 10 UWP 控件库（`Windows.UI.Xaml.Controls`），标出本项目的手搓覆盖进度。

**核心交互控件**

| 控件 | 状态 | 备注 |
|---|:---:|---|
| Button | ✅ | 含 primary 强调色变体 + Reveal 悬停 |
| CheckBox | ✅ | 20px 圆角方框，accent 填充 + 白色勾，hover/check 动画 |
| RadioButton | ✅ | 圆外圈 + 中心 accent 圆点，组内互斥（Core 管理）|
| ToggleSwitch | ✅ | 40×20 轨道 + 12→14px 圆钮（WinUI 3 配色：OFF 白轨道灰钮 / ON accent 白钮）|
| ProgressBar | ✅ | 标签 + 右侧百分比 + 4px 轨道 + accent 填充，value 由 App 驱动 |
| ComboBox | ⏳ | 含弹出层，较复杂 |
| Slider | ✅ | 标签 + 轨道 + accent 已选段 + 可拖拽 thumb（hover 放大）|
| RatingControl | ✅ | 5 颗星（Segoe MDL2 StarFill/Outline）+ hover 预览 + 点选打分 |
| TextBox / PasswordBox | ❌ | 需 IME，手搓最大坑，有意推迟 |
| NumberBox / AutoSuggestBox | ❌ | 低优先级 |
| DatePicker / TimePicker | ❌ | 依赖弹出面板 |
| ProgressRing | ✅ | 底环 + accent 弧（折线逼近，规避 D2D1_ARC）+ 端点圆帽 + 百分比，value 由 App 驱动 |

**数据 / 容器控件**

| 控件 | 状态 | 备注 |
|---|:---:|---|
| ListView / GridView | ⏳ | 路线图"滚动列表"雏形 |
| TreeView | ❌ | |
| DataGrid | ❌ | |
| Expander / Pivot / SplitView | ❌ | |
| ScrollViewer | ⏳ | 依赖滚动列表 |

**结构 / 呈现**

| 区块 | 状态 | 备注 |
|---|:---:|---|
| Card（信息卡） | ✅ | 双列固定 X 网格（标签/值对齐）+ 内容块垂直居中；布局区块，未抽成独立控件 |
| NavPane（侧边导航） | ✅ | 磨砂面板 + 顶部标题 + 三条杠折叠按钮（180↔48px 200ms）+ hover 渐显 + Reveal 光带 + 选中指示条 |
| 标题 / 副标题（TextBlock） | ✅ | |
| Border / Grid / StackPanel | — | 由 Core 布局逻辑直接承担，无独立控件抽象 |

**进度小结**：已覆盖核心交互控件 **Button / CheckBox / RadioButton / ToggleSwitch /
ProgressBar / ProgressRing / Slider / RatingControl** + 导航组件 **NavPane**
+ 非交互区块（Card），其余按上表优先级推进。

### 里程碑

- [ ] 完整版 Reveal（沿边扫过）
- [x] CheckBox / ToggleSwitch / RadioButton / ProgressBar
- [x] ProgressRing / Slider / RatingControl
- [ ] 焦点管理（Tab 导航 + 焦点描边）
- [ ] 滚动条 / 列表（ListView 雏形）
- [ ] 窗口圆角可选（Win10 无圆角，Win11 用 DWM 属性）
- [ ] 可选 UIA 桥接

## 许可

MIT
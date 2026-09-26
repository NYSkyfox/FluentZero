#pragma once

// FluentZero  ——  Win10 风格 Fluent Design，纯手搓，零第三方依赖。
//
// 技术栈（全部是 Windows 系统自带的 DLL）：
//   Win32        窗口 / 消息
//   DWM          Acrylic 模糊（BlurBehind）
//   DirectComposition  独立合成层（HWND 之外自持渲染，支持半透明）
//   Direct2D     全部 UI 绘制
//   DirectWrite  文本排版（Segoe UI / Segoe MDL2 Assets 图标字体）
//
// 目标产物：单个 exe（静态链接 CRT），Win10 1809+ / Win11 均可运行。
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <dcomp.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <algorithm>

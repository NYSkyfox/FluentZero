#pragma once

// 目标平台 Win10：必须在 windows.h 之前定义，
// 否则 d2d1.h/dxgi1_3.h 只暴露旧接口（ID2D1Device 系列 / SetHwnd 均被版本宏门控）
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif

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
#include <d3d11.h>   // 必须在 d2d1.h 之前：D2D1 的部分类型定义依赖 D3D11
#include <dxgi1_3.h> // IDXGISwapChain1 / DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
#include <d2d1.h>
#include <d2d1_1.h>  // D2D1 1.1：ID2D1Device / ID2D1DeviceContext1 / ID2D1Bitmap1
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <algorithm>

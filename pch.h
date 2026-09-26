#pragma once

// 目标平台 Win10：必须在 windows.h 之前定义，
// 否则部分 SDK 头只暴露旧接口
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
//   D2D HwndRenderTarget  窗口内自持渲染（WS_EX_NOREDIRECTIONBITMAP 直连 DWM）
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
#include <dwmapi.h>  // DwmEnableBlurBehindWindow (Acrylic)
#include <d2d1.h>    // D2D1 1.0：HwndRenderTarget / Shared Factory
#include <dwrite.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <algorithm>

using namespace Microsoft::WRL;   // ComPtr<T>（全局使用，避免每处写全名）
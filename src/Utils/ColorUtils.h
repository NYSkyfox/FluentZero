#pragma once

// D2D1 颜色 / 几何工具（依赖 d2d1.h，由 PCH 提供）
namespace fz {

// 非预乘 → 预乘 alpha
D2D1_COLOR_F Premul(D2D1_COLOR_F c);
// 提亮 / 压暗（amt > 0 提亮，< 0 压暗）
D2D1_COLOR_F Brighten(D2D1_COLOR_F c, float amt);
// 构造颜色
D2D1_COLOR_F FzCol(float r, float g, float b, float a = 1.0f);
// 转 #RRGGBB 字符串
std::wstring HexOf(D2D1_COLOR_F c);
// 构造 D2D1_ROUNDED_RECT（C 风格结构体，跨 SDK 版本稳定）
D2D1_ROUNDED_RECT FzRR(float l, float t, float r, float b, float rad);

} // namespace fz
#pragma once

// 基础数学工具（纯数学，无 Win32 依赖）
namespace fz {

inline float FzMx(float a, float b) { return a > b ? a : b; }
inline float Fzmn(float a, float b) { return a < b ? a : b; }
inline float Clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }
inline float EaseOut(float t) { float u = 1 - Clamp01(t); return 1 - u * u * u; }

} // namespace fz
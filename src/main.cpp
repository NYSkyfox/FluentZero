#include "pch.h"
#include "Core/App.h"

// 入口：创建 App → 运行消息循环
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    fz::App app;
    if (FAILED(app.Create())) {
        MessageBoxW(nullptr, L"FluentZero 初始化失败（需要 Win10 1809+）",
                    L"FluentZero", MB_ICONERROR);
        return 1;
    }
    app.Run();
    return 0;
}
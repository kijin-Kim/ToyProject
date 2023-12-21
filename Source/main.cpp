#include <windows.h>
#include "D3D12Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const Application::ApplicationSpec appSpec{};
    D3D12Application App(appSpec);
    App.Run();
}


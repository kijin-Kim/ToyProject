#include <windows.h>
#include "EditorApplication.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const Engine::Application::ApplicationSpec appSpec{};
    EditorApplication App(appSpec);
    App.Run();
}


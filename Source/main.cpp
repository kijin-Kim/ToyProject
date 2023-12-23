#include <windows.h>
#include "EditorApplication.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const Engine::Application::ApplicationSpec appSpec{
        .Width = 1920,
        .Height = 1080,
        .WindowTitle = "Editor"
    };
    EditorApplication App(appSpec);
    App.Run();
}

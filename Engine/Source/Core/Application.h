#pragma once
#include <string>
#include <windows.h>

#include "Timer.h"
#include "Graphics/Renderer.h"

struct GLFWwindow;

namespace Engine
{
    class Application
    {
    public:
        struct ApplicationSpec
        {
            int32_t Width = 800;
            int32_t Height = 600;
            std::string WindowTitle = "Simple Window Application";
        };

    public:
        explicit Application(ApplicationSpec applicationSpec);
        virtual ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        void Run();

        virtual void Initialize();
        virtual void Update();

        HWND GetWindowHandle() const;
        virtual void OnWindowSizeEvent(int width, int height);
        virtual void OnKeyEvent(int key, int scanCode, int action, int mods);
        virtual void OnCursorPosEvent(double xPos, double yPos);
        virtual void OnWindowFocusEvent(bool bIsWindowFocused);
        

    protected:
        Renderer m_Renderer{};
        GLFWwindow* m_Window = nullptr;
        ApplicationSpec m_ApplicationSpec{};
        Timer m_Timer;
        bool m_bIsWindowFocused = false;
    };
}

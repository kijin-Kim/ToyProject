#pragma once
#include <string>
#include <windows.h>

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
        float GetDeltaTime() const;
        void OnWindowSizeChanged(int width, int height);

        
    private:
        float m_DeltaTime = 0.0f;

    protected:
        Renderer m_Renderer{};
        GLFWwindow* m_Window = nullptr;
        ApplicationSpec m_ApplicationSpec{};
    };
}

#pragma once
#include <windows.h>
#include <string>

struct GLFWwindow;

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
    Application(ApplicationSpec applicationSpec);
    virtual ~Application();
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    void Run();
    virtual void Initialize() {};
    virtual void Update();

    HWND GetWindowHandle() const;
    float GetDeltaTime() const;

private:
    GLFWwindow* m_Window = nullptr;
    float m_DeltaTime = 0.0f;

protected:
    ApplicationSpec m_ApplicationSpec{};
};
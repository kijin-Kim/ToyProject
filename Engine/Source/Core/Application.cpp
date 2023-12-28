#include "EnginePCH.h"
#include "Application.h"

#include <spdlog/sinks/msvc_sink.h>

#include "Engine.h"

namespace Engine
{
    Application::Application(ApplicationSpec applicationSpec)
        : m_ApplicationSpec(std::move(applicationSpec))
    {
        auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        const auto logger = std::make_shared<spdlog::logger>("msvc_logger", sink);
        spdlog::set_default_logger(logger);
#ifdef _DEBUG
        spdlog::set_level(spdlog::level::debug);
#else
        spdlog::set_level(spdlog::level::warn);
#endif

        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(m_ApplicationSpec.Width, m_ApplicationSpec.Height, m_ApplicationSpec.WindowTitle.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(m_Window);
    }

    Application::~Application()
    {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void Application::Run()
    {
        Initialize();
        const uint64_t tickPerSec = glfwGetTimerFrequency();
        float lastTime = glfwGetTimerValue() / static_cast<float>(tickPerSec);

        while (!glfwWindowShouldClose(m_Window))
        {
            const float currentTime = glfwGetTimerValue() / static_cast<float>(tickPerSec);
            m_DeltaTime = currentTime - lastTime;
            lastTime = currentTime;
            glfwPollEvents();
            Update();
        }
    }

    void Application::Initialize()
    {
        glfwSetWindowUserPointer(m_Window, this);
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
        {
            Application* application = static_cast<Application*>(glfwGetWindowUserPointer(window));
            application->OnWindowSizeChanged(width, height);
        });

        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && mods == GLFW_MOD_ALT)
            {
                GLFWmonitor* monitor = glfwGetWindowMonitor(window);
                GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);
                glfwSetWindowMonitor(window, monitor == primaryMonitor ? nullptr : primaryMonitor, 0, 0, videoMode->width, videoMode->height, GLFW_DONT_CARE);
            }
        });
        m_Renderer.Initialize(GetWindowHandle(), m_ApplicationSpec.Width, m_ApplicationSpec.Height);
        glfwMaximizeWindow(m_Window);
    }

    void Application::Update()
    {
    }

    HWND Application::GetWindowHandle() const
    {
        return glfwGetWin32Window(m_Window);
    }

    float Application::GetDeltaTime() const
    {
        return m_DeltaTime;
    }

    void Application::OnWindowSizeChanged(int width, int height)
    {
        const GLFWmonitor* monitor = glfwGetWindowMonitor(m_Window);
        m_Renderer.m_SwapChain->SetFullscreenState(monitor ? TRUE : FALSE, nullptr);


        spdlog::info("WindowSizeEvent: Width: {}, Height:{}", width, height);
        if (m_ApplicationSpec.Width != width || m_ApplicationSpec.Height != height)
        {
            m_Renderer.WaitForGPU();
            m_Renderer.m_SwapChainBuffers.clear();
            DXGI_SWAP_CHAIN_DESC swapChainDesc{};
            m_Renderer.m_SwapChain->GetDesc(&swapChainDesc);
            EG_CONFIRM(SUCCEEDED(m_Renderer.m_SwapChain->ResizeBuffers(m_Renderer.m_FrameCount, width, height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags)));


            m_ApplicationSpec.Width = width;
            m_ApplicationSpec.Height = height;
            m_Renderer.m_Width = width;
            m_Renderer.m_Height = height;
            m_Renderer.m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Renderer.m_Width), static_cast<float>(m_Renderer.m_Height));
            m_Renderer.m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<int32_t>(m_Renderer.m_Width), static_cast<int32_t>(m_Renderer.m_Height));

            m_Renderer.CreateRenderTarget();
            m_Renderer.CreateSceneTextures();
        }
    }
}

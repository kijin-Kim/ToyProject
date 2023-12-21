#pragma once

#include "Application.h"
#include "Renderer.h"
#define SPDLOG_USE_STD_FORMAT
#include <spdlog/spdlog.h>


class D3D12Application : public Application
{
public:
    D3D12Application(ApplicationSpec applicationSpec) : Application(std::move(applicationSpec))
    {
    }

    void Initialize() override
    {
#ifdef _DEBUG
        spdlog::set_level(spdlog::level::debug);
#else
        spdlog::set_level(spdlog::level::info);
#endif
        m_Renderer.Initialize(GetWindowHandle(), m_ApplicationSpec.Width, m_ApplicationSpec.Height);
    }

    void Update() override
    {
        m_Renderer.Render();
    }

private:
    Engine::Renderer m_Renderer;
};

#pragma once

#include "Core/Application.h"
#include "Graphics/DynamicDescriptorHeap.h"
#include "Graphics/Renderer.h"


class EditorApplication : public Engine::Application
{
public:
    EditorApplication(ApplicationSpec applicationSpec) : Application(std::move(applicationSpec))
    {
    }

    void Initialize() override;
    void Update() override;

private:
    void RenderUI();


private:
    Engine::Renderer m_Renderer;
    uint32_t m_FrameCount = 2;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_RTVResource = nullptr;
};

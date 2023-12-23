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
    Engine::DynamicDescriptorHeap m_DynamicDescriptorHeap{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 2};
    
};

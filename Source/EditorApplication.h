#pragma once

#include "Core/Application.h"
#include "Graphics/DynamicDescriptorHeap.h"


class EditorApplication : public Engine::Application
{
public:
    explicit EditorApplication(ApplicationSpec applicationSpec) : Application(std::move(applicationSpec))
    {
    }

    ~EditorApplication() override;

    void Initialize() override;
    void Update() override;

private:
    void RenderUI();


private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_RTVResource = nullptr;
};

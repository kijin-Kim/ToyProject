#pragma once

#include "imgui.h"
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
    
    void OnCursorPosEvent(double xPos, double yPos) override;
    void OnWindowFocusEvent(bool bIsWindowFocused) override;

private:
    void RenderUI();


private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_RTVResource = nullptr;
    bool bIsViewportFocused = false;


    DirectX::SimpleMath::Vector3 m_Velocity;
    DirectX::SimpleMath::Vector3 m_Acceleration;
};

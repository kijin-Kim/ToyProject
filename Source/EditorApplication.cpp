#include "EditorApplication.h"

#include <d3dx12.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_glfw.h"
#include "Core/Core.h"
#include "ECS/Components.h"
#include "ImGuizmo/ImGuizmo.h"

EditorApplication::~EditorApplication()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorApplication::Initialize()
{
    Application::Initialize();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;


    // Setup Dear ImGui style


    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(m_Window, true);


    // TODO:
    ImGui_ImplDX12_Init(Engine::Core::GetRenderContext().GetDevice().Get(), m_Renderer.m_FrameCount,
                        DXGI_FORMAT_R8G8B8A8_UNORM, m_Renderer.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).Get(),
                        m_Renderer.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetCPUDescriptorHandleForHeapStart(),
                        m_Renderer.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetGPUDescriptorHandleForHeapStart());

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameBorderSize = 1.0f;
    style.FrameRounding = 4.0f;

    ImFontConfig fontConfig;
    fontConfig.RasterizerDensity = 1.25f;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;

    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("Content/Fonts/NanumFontSetup_TTF_SQUARE/NanumSquareB.ttf", 13, &fontConfig);
}

void EditorApplication::HandleCameraMovement()
{
    if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) && m_bIsViewportFocused)
    {
        if (glfwGetKey(m_Window, GLFW_KEY_W))
        {
            m_Velocity = m_Renderer.m_CameraTransform.Forward() * 80.0f;
        }

        if (glfwGetKey(m_Window, GLFW_KEY_S))
        {
            m_Velocity = m_Renderer.m_CameraTransform.Backward() * 80.0f;
        }
        if (glfwGetKey(m_Window, GLFW_KEY_D))
        {
            m_Velocity = m_Renderer.m_CameraTransform.Right() * 80.0f;
        }
        if (glfwGetKey(m_Window, GLFW_KEY_A))
        {
            m_Velocity = m_Renderer.m_CameraTransform.Left() * 80.0f;
        }
        if (glfwGetKey(m_Window, GLFW_KEY_E))
        {
            m_Velocity = m_Renderer.m_CameraTransform.Up() * 80.0f;
        }
        if (glfwGetKey(m_Window, GLFW_KEY_Q))
        {
            m_Velocity = m_Renderer.m_CameraTransform.Down() * 80.0f;
        }
    }


    m_Velocity *= 0.96f;
    auto& cameraTransform = m_Renderer.m_Registry.get<Engine::TransformComponent>(m_Renderer.m_CameraEntity);
    cameraTransform.Position += m_Velocity * static_cast<float>(m_Timer.GetDeltaSeconds());
}

void EditorApplication::Update()
{
    HandleCameraMovement();
    m_Renderer.SubmitGraphicsCommand([this](ID3D12GraphicsCommandList* commandList)
    {
        this->RenderUI();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    });

    m_Renderer.Render();
}

void EditorApplication::OnCursorPosEvent(double xPos, double yPos)
{
    Application::OnCursorPosEvent(xPos, yPos);

    static bool bWasRightButton = false;
    static double lastX = 0;
    static double lastY = 0;


    if (!glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) || !m_bIsViewportFocused)
    {
        bWasRightButton = false;
        lastX = xPos;
        lastY = yPos;
        return;
    }


    const float deltaX = static_cast<float>(lastX - xPos);
    const float deltaY = static_cast<float>(lastY - yPos);

    static float yaw = 0;
    static float pitch = 0;
    yaw += deltaX * 15.0f * static_cast<float>(m_Timer.GetDeltaSeconds());
    pitch += deltaY * 15.0f * static_cast<float>(m_Timer.GetDeltaSeconds());

    auto& cameraTransform = m_Renderer.m_Registry.get<Engine::TransformComponent>(m_Renderer.m_CameraEntity);
    cameraTransform.Rotation = DirectX::SimpleMath::Vector3{pitch, yaw, 0.0f};


    lastX = xPos;
    lastY = yPos;
}

void EditorApplication::OnWindowFocusEvent(bool bIsWindowFocused)
{
    Application::OnWindowFocusEvent(bIsWindowFocused);
}

void EditorApplication::OnScrollEvent(double xOffset, double yOffset)
{
    Application::OnScrollEvent(xOffset, yOffset);
    if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT) && m_bIsViewportFocused)
    {
        m_Velocity = m_Renderer.m_CameraTransform.Backward() * 80.0f * 1.5f;
        m_Velocity = yOffset > 0 ? m_Velocity : -m_Velocity;
    }
}

void EditorApplication::RenderUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();


    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                glfwSetWindowShouldClose(m_Window, true);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::DockSpaceOverViewport();


    ImGui::Begin("Content Browser");
    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered())
    {
        ImGui::SetWindowFocus();
    }

    ImGui::End();


    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    bool show_demo_window;
    ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("Viewport", nullptr, windowFlags);
    auto descHeap = m_Renderer.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(descHeap->GetGPUDescriptorHandleForHeapStart(), m_Renderer.GetCurrentBackBufferIndex() + 1, Engine::Core::GetRenderContext().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    ImGui::Image((void*)gpuHandle.ptr, ImGui::GetWindowSize());
    m_Renderer.m_AspectRatio = ImGui::GetWindowSize().x / ImGui::GetWindowSize().y;

    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered())
    {
        ImGui::SetWindowFocus();
    }
    if (ImGui::IsWindowFocused())
    {
        m_bIsViewportFocused = true;
    }
    else
    {
        m_bIsViewportFocused = false;
    }


    ImGuizmo::AllowAxisFlip(false);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());


    static ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_W))
        operation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(ImGuiKey_E))
        operation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(ImGuiKey_R))
        operation = ImGuizmo::SCALE;

    
    
    ImGuizmo::SetGizmoSizeClipSpace(0.2f);
    Manipulate(m_Renderer.m_CameraTransform.Invert().m[0], m_Renderer.m_Projection.m[0], operation, ImGuizmo::LOCAL, m_Renderer.m_Model.m[0]);

    ImGui::End();
    ImGui::PopStyleVar(1);


    ImGui::Begin("Hierarchy");
    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered())
    {
        ImGui::SetWindowFocus();
    }

    const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    auto& registry = m_Renderer.GetRegister();
    static entt::entity selectedEntity = entt::null;


    if (ImGui::TreeNodeEx("Scene", treeNodeFlags | ImGuiTreeNodeFlags_DefaultOpen))
    {
        auto view = registry.view<Engine::DisplayNameComponent>();
        for (const auto [entity, displayName] : view.each())
        {
            ImGui::PushID(entt::to_integral(entity));
            bool bOpened = ImGui::TreeNodeEx(displayName.Name.c_str(),
                                             selectedEntity == entity ? treeNodeFlags | ImGuiTreeNodeFlags_Selected : treeNodeFlags);

            if (ImGui::IsItemClicked())
            {
                selectedEntity = entity;
            }
            if (bOpened)
            {
                // TODO : 자식 Entity
                ImGui::TreePop();
            }
            ImGui::PopID();
        }

        ImGui::TreePop();
    }
    ImGui::End();

    ImGui::Begin("Inspector");
    if (ImGui::IsMouseClicked(1) && ImGui::IsWindowHovered())
    {
        ImGui::SetWindowFocus();
    }

    ImGui::PushID(entt::to_integral(selectedEntity));
    if (registry.all_of<Engine::DisplayNameComponent>(selectedEntity))
    {
        if (ImGui::CollapsingHeader("DisplayName", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            auto& displayName = registry.get<Engine::DisplayNameComponent>(selectedEntity);
            displayName.Name.resize(32);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
            ImGui::Indent();
            ImGui::Text("DisplayName");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##Name", displayName.Name.data(), displayName.Name.size());
            ImGui::PopItemWidth();
            ImGui::Unindent();
            ImGui::PopStyleVar();
            ImGui::Spacing();
        }
    }
    if (registry.all_of<Engine::TransformComponent>(selectedEntity))
    {
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
            auto& transform = registry.get<Engine::TransformComponent>(selectedEntity);
            ImGui::Indent();
            ImGui::Text("Position");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat3("##P", &transform.Position.x);
            ImGui::PopItemWidth();

            ImGui::Text("Rotation");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat3("##R", &transform.Rotation.x);
            ImGui::PopItemWidth();

            ImGui::Text("Scale");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat3("##S", &transform.Scale.x);
            ImGui::PopItemWidth();

            ImGui::Unindent();
            ImGui::PopStyleVar();
            ImGui::Spacing();
        }
    }

    if (registry.all_of<Engine::LightComponent>(selectedEntity))
    {
        if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));
            auto& light = registry.get<Engine::LightComponent>(selectedEntity);
            ImGui::Indent();
            ImGui::Text("Light Color");
            ImGui::SameLine(ImGui::GetContentRegionAvail().x * 0.5f);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::ColorEdit3("##C", &light.LightColor.x);
            ImGui::PopItemWidth();

            ImGui::Unindent();
            ImGui::PopStyleVar();
            ImGui::Spacing();
        }
    }

    ImGui::PopID();


    ImGui::End();
    ImGui::Render();
}

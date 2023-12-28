#include "EditorApplication.h"

#include <d3dx12.h>
#include <spdlog/spdlog.h>
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_glfw.h"
#include "Core/Core.h"
#include "ECS/Components.h"

EditorApplication::~EditorApplication()
{
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorApplication::Initialize()
{
    Application::Initialize();
    // Setup Dear ImGui context
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

    ImFontConfig fontConfig;
    fontConfig.RasterizerDensity = 1.25f;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;

    io.Fonts->Clear();
    io.Fonts->AddFontFromFileTTF("Content/Fonts/NanumFontSetup_TTF_SQUARE/NanumSquareB.ttf", 13, &fontConfig);
}

void EditorApplication::Update()
{
    m_Renderer.SubmitGraphicsCommand([this](ID3D12GraphicsCommandList* commandList)
    {
        this->RenderUI();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
    });


    m_Renderer.Render();
}

void EditorApplication::RenderUI()
{
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport();

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    bool show_demo_window;
    bool show_another_window;
    ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
        //		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
        ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);
        // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
    // Rendering

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar;
    ImGui::Begin("Viewport", nullptr, windowFlags);
    auto descHeap = m_Renderer.GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(descHeap->GetGPUDescriptorHandleForHeapStart(), m_Renderer.GetCurrentBackBufferIndex() + 1,
                                            Engine::Core::GetRenderContext().GetDevice()->GetDescriptorHandleIncrementSize(
                                                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    ImGui::Image((void*)gpuHandle.ptr, ImGui::GetWindowSize());
    ImGui::End();
    ImGui::PopStyleVar(1);


    ImGui::Begin("Hierarchy");


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

    ImGui::PushID(entt::to_integral(selectedEntity));
    if (registry.all_of<Engine::DisplayNameComponent>(selectedEntity))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        if (ImGui::CollapsingHeader("DisplayName", ImGuiTreeNodeFlags_DefaultOpen))
        {
            
            auto& displayName = registry.get<Engine::DisplayNameComponent>(selectedEntity);
            displayName.Name.resize(32);
            ImGui::InputText("Name", displayName.Name.data(), displayName.Name.size());
        }
        ImGui::PopStyleVar();
        
    }

    if (registry.all_of<Engine::TransformComponent>(selectedEntity))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto& transform = registry.get<Engine::TransformComponent>(selectedEntity);
            ImGui::DragFloat3("Position", &transform.Position.x);
            
        }
        ImGui::PopStyleVar();
    }
    ImGui::PopID();

    ImGui::End();


    ImGui::Render();
}

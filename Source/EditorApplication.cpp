#include "EditorApplication.h"
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_glfw.h"
#include "Core/Core.h"

void EditorApplication::Initialize()
{
    m_Renderer.Initialize(GetWindowHandle(), m_ApplicationSpec.Width, m_ApplicationSpec.Height);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();


    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(m_Window, true);


    // TODO:
    ImGui_ImplDX12_Init(Engine::Core::GetRenderContext().GetDevice(), m_FrameCount,
                        DXGI_FORMAT_R8G8B8A8_UNORM, m_DynamicDescriptorHeap.GetD3D12DescriptorHeap(),
                        m_DynamicDescriptorHeap.GetD3D12DescriptorHeap()->GetCPUDescriptorHandleForHeapStart(),
                        m_DynamicDescriptorHeap.GetD3D12DescriptorHeap()->GetGPUDescriptorHandleForHeapStart());
}

void EditorApplication::Update()
{
    m_Renderer.SubmitGraphicsCommand([this](ID3D12GraphicsCommandList* commandList)
    {
        this->RenderUI();

        ID3D12DescriptorHeap* descriptorHeaps[] = {m_DynamicDescriptorHeap.GetD3D12DescriptorHeap()};
        commandList->SetDescriptorHeaps(1, descriptorHeaps);
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
    
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    bool show_demo_window;
    bool show_another_window;
    ImGui::ShowDemoWindow(&show_demo_window);
    //ImGui::DockSpace()

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
    ImGui::Render();
}

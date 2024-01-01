#pragma once
#include <array>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <vector>
#include <wrl.h>

#include "AssetManager.h"
#include "GraphicsTypes.h"
#include "SimpleMath.h"
#include "ECS/Scene.h"

struct CD3DX12_ROOT_PARAMETER;
namespace Engine
{
    struct DisplayNameComponent;


    class Renderer
    {
        // TEMP
        friend class Application;

    public:
        using RenderCommand = std::function<void(ID3D12GraphicsCommandList*)>;

    public:
        void Initialize(HWND windowHandle, uint32_t width, uint32_t height);

        
        void InitDirectX(HWND windowHandle);
        void Prepare();
        void Render();

        void CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& outCommandQueue);
        void CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& outCommandAllocator);
        void CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& outCommandList);
        void CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlag, uint32_t maximumDescriptorsCount, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& outDescriptorHeap);

        
        void CreateSwapChain(uint32_t width, uint32_t height, HWND windowHandle);
        void CreateRenderTarget(uint32_t width, uint32_t height);
        void CreateSceneTextures(uint64_t width, uint32_t height);
        void CreateFence();
        void WaitForGPU();


        void CreateVertexAndIndexBufferView(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices,
                                            Microsoft::WRL::ComPtr<ID3D12Resource>& outVertexBuffer, Microsoft::WRL::ComPtr<ID3D12Resource>& outIndexBuffer,
                                            D3D12_VERTEX_BUFFER_VIEW& outVertexBufferView, D3D12_INDEX_BUFFER_VIEW& outIndexBufferView);
        void CreateRootSignature(uint32_t rootParameterCount, CD3DX12_ROOT_PARAMETER* rootParameters, Microsoft::WRL::ComPtr<ID3D12RootSignature>& outRootSignature);
        void CreatePipelineState();

        
        void LoadAssets();
        void SubmitGraphicsCommand(const RenderCommand& renderCommand);

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE DescHeapType) const
        {
            return m_DescriptorHeaps[DescHeapType];
        }


        uint32_t GetCurrentBackBufferIndex() const
        {
            return m_SwapChain->GetCurrentBackBufferIndex();
        }

        entt::registry& GetRegister() { return m_Registry; }

    public:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_DirectCommandQueue = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_DirectCommandAllocator = nullptr;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_DirectCommandList = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CopyCommandQueue = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CopyCommandAllocator = nullptr;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CopyCommandList = nullptr;

        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain = nullptr;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_SwapChainBuffers;
        std::array<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeaps;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};

        Microsoft::WRL::ComPtr<ID3D12Resource> m_BillboardVertexBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_BillboardIndexBuffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_BillboardVertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_BillboardIndexBufferView{};

        

        D3D12_VIEWPORT m_Viewport{};
        D3D12_RECT m_ScissorRect{};

        

        
        Microsoft::WRL::Wrappers::Event m_FenceEvent;
        uint64_t m_FenceValue = 0;
        uint32_t m_Width = 1366;
        uint32_t m_Height = 768;


        Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;

        std::vector<RenderCommand> m_RenderCommands;

        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_SceneColorBuffers;


        entt::registry m_Registry;

        const uint32_t m_FrameCount = 2;

        DirectX::SimpleMath::Matrix m_Model = DirectX::SimpleMath::Matrix::Identity;
        DirectX::SimpleMath::Matrix m_CameraTransform;
        DirectX::SimpleMath::Matrix m_Projection;

        

        float m_AspectRatio;
        float m_FieldOfView;

        std::unique_ptr<Scene> m_Scene;

        entt::entity m_CameraEntity;
        
    };
    
}

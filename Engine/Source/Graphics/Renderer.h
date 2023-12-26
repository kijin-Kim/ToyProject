#pragma once
#include <array>
#include <d3d12.h>
#include <dxgi1_5.h>
#include <vector>
#include <wrl.h>

#include "AssetManager.h"
#include "GraphicsTypes.h"
#include "SimpleMath.h"


namespace Engine
{
    class Renderer
    {
    public:
        using RenderCommand = std::function<void(ID3D12GraphicsCommandList*)>;

    public:
        void Initialize(HWND windowHandle, uint32_t width, uint32_t height);
        void InitDirectX(HWND windowHandle);
        void Prepare();
        void Render();

        void CreateCommandQueue();
        void CreateCommandList();
        void CreateDescriptorHeaps();
        void CreateSwapChain(HWND windowHandle);
        void CreateRenderTarget();
        void CreateSceneTextures();
        void CreateFence();
        void WaitForGPU();
        void CreateVertexAndIndexBuffer();
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
        

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator = nullptr;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CopyCommandQueue = nullptr;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CopyCommandAllocator = nullptr;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CopyCommandList = nullptr;

        Microsoft::WRL::ComPtr<IDXGISwapChain3> m_SwapChain = nullptr;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_SwapChainBuffers;
        std::array<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeaps;
        std::array<uint32_t, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeapIncrementSizes;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_VertexBuffer = nullptr;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};

        D3D12_VIEWPORT m_Viewport{};
        D3D12_RECT m_ScissorRect{};
        Microsoft::WRL::Wrappers::Event m_FenceEvent;
        uint64_t m_FenceValue = 0;
        uint32_t m_Width = 1366;
        uint32_t m_Height = 768;


        Microsoft::WRL::ComPtr<ID3D12Resource> m_Texture;
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;

        std::vector<RenderCommand> m_RenderCommands;

        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_SceneColorBuffers;
    };
}

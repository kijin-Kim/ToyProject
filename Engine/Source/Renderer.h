#pragma once
#include <array>
#include <wrl.h>
#include <dxgi1_6.h>
#include <vector>
#include <d3d12.h>

#include "RendererTypes.h"
#include "ResourceManager.h"
#include "SimpleMath.h"


namespace Engine
{
    using namespace Microsoft::WRL;
    using namespace DirectX;

    class Renderer
    {
    public:
        void Initialize(HWND windowHandle, uint32_t width, uint32_t height);
        void InitDirectX(HWND windowHandle);
        void Prepare();
        void Render();

        void EnableDebugLayer();
        void CreateFactory();
        void CreateDevice();
        void CreateCommandQueue();
        void CreateCommandList();
        void CreateDescriptorHeaps();
        void CreateSwapChain(HWND windowHandle);
        void CreateRenderTarget();
        void CreateFence();
        void WaitForGPU();
        void CreateVertexAndIndexBuffer();
        void CreateTextureShaderResourceView(entt::resource<Texture> textureHandle);
        void CreatePipelineState();

        void LoadAssets();

    private:
        ComPtr<IDXGIFactory5> m_Factory = nullptr;
        ComPtr<ID3D12Device4> m_Device = nullptr;
        ComPtr<ID3D12Fence> m_Fence = nullptr;
        ComPtr<ID3D12CommandQueue> m_CommandQueue = nullptr;
        ComPtr<ID3D12CommandAllocator> m_CommandAllocator = nullptr;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList = nullptr;

        ComPtr<IDXGISwapChain3> m_SwapChain = nullptr;
        std::vector<ComPtr<ID3D12Resource>> m_SwapChainBuffers;
        std::array<ComPtr<ID3D12DescriptorHeap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeaps;
        std::array<uint32_t, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorHeapIncrementSizes;

        ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
        ComPtr<ID3D12PipelineState> m_PipelineState = nullptr;

        ComPtr<ID3D12Resource> m_DepthStencilBuffer = nullptr;
        ComPtr<ID3D12Resource> m_VertexBuffer = nullptr;
        ComPtr<ID3D12Resource> m_IndexBuffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};

        D3D12_VIEWPORT m_Viewport{};
        D3D12_RECT m_ScissorRect{};
        Wrappers::Event m_FenceEvent;
        uint64_t m_FenceValue;
        uint32_t m_Width = 1366;
        uint32_t m_Height = 768;
        float m_DeltaTime = 0.0f;


        ComPtr<ID3D12Resource> m_Texture;
        std::vector<Vertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
    };
}

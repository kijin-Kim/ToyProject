#pragma once

namespace Engine
{
    class RenderContext final
    {
        friend class Core;

    public:
        Microsoft::WRL::ComPtr<ID3D12Device4> GetDevice() const { return m_Device; }
        Microsoft::WRL::ComPtr<IDXGIFactory5> GetFactory() const { return m_Factory; }
        uint32_t GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const { return m_DescriptorIncrementSizes[descriptorHeapType]; }

    private:
        RenderContext();
        void EnableDebugLayer();
        void CreateFactory();
        void CreateDevice();
        void RetrieveDescriptorIncrementSizes();

    private:
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device = nullptr;
        Microsoft::WRL::ComPtr<IDXGIFactory5> m_Factory = nullptr;
        std::array<uint32_t, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_DescriptorIncrementSizes;
    };
}

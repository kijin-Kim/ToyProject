#pragma once


namespace Engine
{
    class DynamicDescriptorHeap
    {
    public:
        DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t maxDescriptorCount);
        virtual ~DynamicDescriptorHeap() = default;

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorHeapType;
        uint32_t m_MaxDescriptorCount;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap = nullptr;
    };
}


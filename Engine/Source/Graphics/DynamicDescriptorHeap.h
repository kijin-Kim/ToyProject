#pragma once
#include <d3d12.h>
#include <wrl.h>


namespace Engine
{
    class DynamicDescriptorHeap
    {
    public:
        DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t maxDescriptorCount);
        virtual ~DynamicDescriptorHeap() = default;
        
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return m_DescriptorHeap.Get(); }
        
    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorHeapType;
        uint32_t m_MaxDescriptorCount;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap = nullptr;
                

    };
}


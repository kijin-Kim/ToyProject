#include "EnginePCH.h"
#include "DynamicDescriptorHeap.h"

#include "Engine.h"
#include "Core/Core.h"

namespace Engine
{
    DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, uint32_t maxDescriptorCount) :
        m_DescriptorHeapType(descriptorHeapType), m_MaxDescriptorCount(maxDescriptorCount)
    {
        EG_CONFIRM(m_DescriptorHeapType < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES && m_MaxDescriptorCount > 0);

        const D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{
            .Type = m_DescriptorHeapType,
            .NumDescriptors = m_MaxDescriptorCount,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask = 0
        };
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeap))));
    }
}

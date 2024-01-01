#include "EnginePCH.h"
#include "GraphicsHelper.h"

#include <wrl/wrappers/corewrappers.h>

#include "Engine.h"
#include "Texture.h"
#include "Core/Core.h"

namespace Engine
{
    namespace GraphicsHelper
    {
        void CreateTextureResource(entt::resource<Texture> textureHandle,
                                   Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
                                   Microsoft::WRL::ComPtr<ID3D12Fence> fence,
                                   uint64_t& fenceValue,
                                   Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
                                   Microsoft::WRL::ComPtr<ID3D12Resource>& outResource)
        {
            D3D12_SUBRESOURCE_DATA subresourceData = {};
            subresourceData.pData = textureHandle->Data;
            subresourceData.RowPitch = textureHandle->Width * textureHandle->channelCount;
            subresourceData.SlicePitch = textureHandle->Width * textureHandle->Height * textureHandle->channelCount;

            const auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 512, 512);
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

            EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(outResource.GetAddressOf()))));

            const auto uploadBufferSize = GetRequiredIntermediateSize(outResource.Get(), 0, 1);

            CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
            Microsoft::WRL::ComPtr<ID3D12Resource> textureUploadHeap;
            EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferResourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&textureUploadHeap))));


            UpdateSubresources(commandList.Get(), outResource.Get(), textureUploadHeap.Get(), 0, 0, 1, &subresourceData);
            CD3DX12_RESOURCE_BARRIER barrier{};
            barrier = CD3DX12_RESOURCE_BARRIER::Transition(outResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            commandList->ResourceBarrier(1, &barrier);
            commandList->Close();
            commandQueue->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));
            WaitForGPU(commandQueue, fence, fenceValue);
        }

        void WaitForGPU(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, Microsoft::WRL::ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
        {
            // WaitGPU
            commandQueue->Signal(fence.Get(), fenceValue);
            if (fence->GetCompletedValue() < fenceValue)
            {
                Microsoft::WRL::Wrappers::Event fenceEvent;
                EG_CONFIRM(SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent.Get())));
                WaitForSingleObject(fenceEvent.Get(), INFINITE);
            }
            fenceValue++;
        }
    }
}

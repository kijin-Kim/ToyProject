#pragma once

namespace Microsoft::WRL::Wrappers
{
    class Event;
}

namespace Engine
{
    struct Texture;

    namespace GraphicsHelper
    {
        void CreateTextureResource(entt::resource<Texture> textureHandle,
                                   Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue,
                                   Microsoft::WRL::ComPtr<ID3D12Fence> fence,
                                   uint64_t& fenceValue,
                                   Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
                                   Microsoft::WRL::ComPtr<ID3D12Resource>& outResource);

        void WaitForGPU(Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, Microsoft::WRL::ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
    }
}

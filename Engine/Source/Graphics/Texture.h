#pragma once
#include <memory>

namespace Engine
{
    struct Texture
    {
        uint8_t* Data;
        uint32_t Width;
        uint32_t Height;
        uint32_t channelCount;

        Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHandle;
    };

    class TextureLoader
    {
    public:
        using result_type = std::shared_ptr<Texture>;
        result_type operator()(std::string_view filePath) const;
    };
}

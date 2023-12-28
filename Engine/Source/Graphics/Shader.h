#pragma once

namespace Engine
{
    enum class ShaderType
    {
        Vertex,
        Pixel
    };
    
    struct Shader
    {
        Microsoft::WRL::ComPtr<ID3D10Blob> Blob = nullptr;
    };

    class ShaderLoader
    {
    public:
        using result_type = std::shared_ptr<Shader>;
        result_type operator()(std::wstring_view filePath, ShaderType shaderType) const;
    };
}

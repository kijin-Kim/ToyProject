#include "EnginePCH.h"
#include "Shader.h"

#include "Engine.h"

namespace Engine
{
    ShaderLoader::result_type ShaderLoader::operator()(std::wstring_view filePath, ShaderType shaderType) const
    {
        std::string shaderTypeString;
        switch (shaderType)
        {
        case ShaderType::Vertex: shaderTypeString = "vs";
            break;
        case ShaderType::Pixel: shaderTypeString = "ps";
            break;
        default: EG_CONFIRM(false);
        }
        shaderTypeString += "_5_1";

        uint32_t compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#ifdef _DEBUG
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_PARTIAL_PRECISION | D3DCOMPILE_SKIP_VALIDATION | D3DCOMPILE_AVOID_FLOW_CONTROL; 
#endif


        Microsoft::WRL::ComPtr<ID3D10Blob> dataBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3D10Blob> errorBlob = nullptr;
        const bool bCompileSucceeded = SUCCEEDED(D3DCompileFromFile(filePath.data(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", shaderTypeString.c_str(), compileFlags, 0, dataBlob.GetAddressOf(), errorBlob.GetAddressOf()));
        const bool bHasError = errorBlob && errorBlob->GetBufferSize() > 0;
        if (bHasError || !bCompileSucceeded)
        {
            const std::string_view msg{static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize()};
            spdlog::warn(msg);
            EG_CONFIRM(bCompileSucceeded);
        }

        return std::make_shared<Shader>(dataBlob);
    }
}

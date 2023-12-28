#pragma once
#include <entt/entt.hpp>

#include "Graphics/Shader.h"


namespace Engine
{
    struct Mesh;
    struct Texture;

    class AssetManager
    {
    public:
        static entt::resource<Shader> LoadShader(const entt::id_type id, std::wstring_view filePath, ShaderType shaderType);
        static entt::resource<Texture> LoadTexture(const entt::id_type id, std::string_view filePath);
        static entt::resource<Mesh> LoadMesh(const entt::id_type id, std::string_view filePath);
    };
}

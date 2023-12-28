#include "EnginePCH.h"
#include "AssetManager.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"

namespace Engine
{
    entt::resource<Shader> AssetManager::LoadShader(const entt::id_type id, std::wstring_view filePath, ShaderType shaderType)
    {
        static entt::resource_cache<Shader, ShaderLoader> resourceCache{};
        return resourceCache.load(id, filePath, shaderType).first->second;
    }

    entt::resource<Texture> AssetManager::LoadTexture(const entt::id_type id, std::string_view filePath)
    {
        static entt::resource_cache<Texture, TextureLoader> resourceCache{};
        return resourceCache.load(id, filePath).first->second;
    }

    entt::resource<Mesh> AssetManager::LoadMesh(const entt::id_type id, std::string_view filePath)
    {
        static entt::resource_cache<Mesh, MeshLoader> resourceCache{};
        return resourceCache.load(id, filePath).first->second;
    }
}

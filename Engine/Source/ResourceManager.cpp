#include "ResourceManager.h"

#include "Mesh.h"
#include "Texture.h"

namespace Engine
{
    entt::resource<Texture> ResourceManager::LoadTexture(const entt::id_type id, std::string_view filePath)
    {
        static entt::resource_cache<Texture, TextureLoader> resourceCache{};
        return resourceCache.load(id, filePath).first->second;
    }

    entt::resource<Mesh> ResourceManager::LoadMesh(const entt::id_type id, std::string_view filePath)
    {
        static entt::resource_cache<Mesh, MeshLoader> resourceCache{};
        return resourceCache.load(id, filePath).first->second;
    }
}

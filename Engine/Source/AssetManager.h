#pragma once
#include <entt/entt.hpp>


namespace Engine
{
    struct Mesh;
    struct Texture;

    class AssetManager
    {
    public:
        static entt::resource<Texture> LoadTexture(const entt::id_type id, std::string_view filePath);
        static entt::resource<Mesh> LoadMesh(const entt::id_type id, std::string_view filePath);
    };
}

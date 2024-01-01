#pragma once

namespace Engine
{
    class Entity;

    class Scene
    {
    public:
        Entity SpawnEntity();
        template <typename T, typename... Args>
        T& AddComponentToEntity(entt::entity entityHandle, Args&& ...args)
        {
            return m_Registry.emplace<T>(entityHandle, std::forward<Args>(args)...);
        }

    private:
        entt::registry m_Registry;
    };
}

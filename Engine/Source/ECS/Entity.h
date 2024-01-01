#pragma once
#include "Scene.h"

namespace Engine
{
    class Entity
    {
    public:
        explicit Entity(entt::entity entityHandle, Scene* scene);

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            return m_Scene->AddComponentToEntity<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

    private:
        entt::entity m_EntityHandle;
        Scene* m_Scene = nullptr;
    };
    
}

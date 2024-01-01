#include "EnginePCH.h"
#include "Scene.h"
#include "Entity.h"

namespace Engine
{
    Entity Scene::SpawnEntity()
    {
        return Entity(m_Registry.create(), this);
    }
}

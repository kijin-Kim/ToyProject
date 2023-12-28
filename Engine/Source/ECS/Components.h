#pragma once
#include <SimpleMath.h>

namespace Engine
{
    struct DisplayNameComponent
    {
        std::string Name;
    };

    
    struct TransformComponent
    {
        DirectX::SimpleMath::Vector3 Position{0.0f, 0.0f, 0.0f};
        DirectX::SimpleMath::Vector3 Rotation{0.0f, 0.0f, 0.0f};
        DirectX::SimpleMath::Vector3 Scale{1.0f, 1.0f, 1.0f};
    };

    struct LightComponent
    {
        DirectX::SimpleMath::Color LightColor{1.0f, 1.0f, 1.0f};
    };
}

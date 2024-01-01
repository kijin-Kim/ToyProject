#pragma once
#include "SimpleMath.h"

namespace Engine
{
    struct Vertex
    {
        DirectX::SimpleMath::Vector3 Position{};
        DirectX::SimpleMath::Vector3 Normal{};
        DirectX::SimpleMath::Vector2 TexCoord{};
    };
}

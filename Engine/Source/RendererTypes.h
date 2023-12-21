#pragma once
#include "SimpleMath.h"

namespace Engine
{
    using namespace DirectX;
    struct Vertex
    {
        SimpleMath::Vector3 Position;
        SimpleMath::Vector3 Normal;
        SimpleMath::Vector2 TexCoord;
    };
}

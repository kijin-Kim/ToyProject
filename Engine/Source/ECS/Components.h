#pragma once
#include <SimpleMath.h>

#include "Graphics/Mesh.h"

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
    
    struct MeshRenderComponent
    {
        // 정확히는 MeshComponent가 Vertex에 대한 정보를 가지고 있을 이유가 없음.
        // 왜냐하면 Renderer가 모든 Vertex를 보면서 작업을 하는 일은 거의 없음.
        // 그것보다는 Renderer가 각각의 Mesh Vertex를 유니크하게 저장하고,
        // Instance Drawing하는 것이 맞음.
        // 결론적으로, Renderer는 Renderer만의 Mesh 자료구조가 필요함.
        // 오히려 Renderer는 IASetVertexBuffer를 통해 VertexBuffer를 Set하는것 보다는
        // Lighting, Shadow, Color와 같은 Material. Instancing할 때, 루프를 돌면서 버퍼에 업로드해야하는
        // 정보에 관심이 많음.
        entt::resource<Mesh> Mesh;
        
    };
    
}

#pragma once
#include <memory>
#include <vector>
#include "GraphicsTypes.h"


namespace Engine
{
    
    struct Mesh
    {
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;
        
    };


    struct MeshLoader
    {
        using result_type = std::shared_ptr<Mesh>;
        result_type operator()(std::string_view filePath) const;
    };
}

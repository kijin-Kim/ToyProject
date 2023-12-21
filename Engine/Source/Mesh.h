#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "RendererTypes.h"


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

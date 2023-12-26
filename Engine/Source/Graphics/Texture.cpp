#include "EnginePCH.h"
#include "Texture.h"
#include <stb_image.h>
#include <entt/entt.hpp>

namespace Engine
{
    TextureLoader::result_type TextureLoader::operator()(std::string_view filePath) const
    {
        int width = 0;
        int height = 0;
        int channelCount = 0;
        uint8_t* data = stbi_load(filePath.data(), &width, &height, &channelCount, 0);
        return std::make_shared<Texture>(data, width, height, channelCount);
    }

}


#pragma once
#include <memory>

namespace Engine
{
    struct Texture
    {
        uint8_t* Data;
        uint32_t Width;
        uint32_t Height;
        uint32_t channelCount;
    };

    class TextureLoader
    {
    public:
        using result_type = std::shared_ptr<Texture>;
        result_type operator()(std::string_view filePath) const;
    };
}

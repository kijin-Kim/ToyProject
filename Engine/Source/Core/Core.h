#pragma once
#include "RenderContext.h"


namespace Engine
{
    class Core final
    {
    public:
        Core() = delete;
        static const RenderContext& GetRenderContext() { return m_RenderContext; }

    private:
        static RenderContext m_RenderContext;
    };
}

#pragma once
#include <cstdint>

namespace anv
{
    struct RenderFrameContext
    {
        uint32_t imageIndex = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };
}


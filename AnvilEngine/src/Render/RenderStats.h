#pragma once
#include <cstdint>

namespace anv
{
    struct RendererStats
    {
        uint32_t DrawCalls = 0;

        uint32_t QuadCount = 0;

        uint32_t VertexCount = 0;
        uint32_t IndexCount = 0;

        float FrameTime = 0.0f;
        float FPS = 0.0f;
    };
}
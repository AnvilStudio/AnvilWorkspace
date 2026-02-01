#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace anv
{
    using TextureHandle = uint64_t; 
    using MaterialHandle = uint64_t;

    struct RectUV {
        glm::vec2 min{ 0,0 };
        glm::vec2 max{ 1,1 };
    };

    struct SpriteDrawItem {
        TextureHandle texture = 0;   
        RectUV uv{};
        glm::mat4 model{ 1.0f };
        glm::vec4 color{ 1,1,1,1 };
        float layer = 0.0f;
    };

    struct RenderList2D {
        glm::mat4 viewProj{ 1.0f };
        const SpriteDrawItem* sprites = nullptr;
        uint32_t spriteCount = 0;
    };
}

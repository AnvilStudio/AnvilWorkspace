#pragma once
#include <glm/glm.hpp>
#include <Core/Reference.h>

namespace anv
{
    class Texture;

    struct QuadVertex
    {
        glm::vec2 Position;
        glm::vec2 TexCoord;
    };

    struct Quad
    {
        static inline QuadVertex vertices[4] =
        {
            {{-0.5f, -0.5f}, {0.0f, 0.0f}},
            {{ 0.5f, -0.5f}, {1.0f, 0.0f}},
            {{ 0.5f,  0.5f}, {1.0f, 1.0f}},
            {{-0.5f,  0.5f}, {0.0f, 1.0f}}
        };

        static inline uint32_t indices[6] =
        {
            0, 1, 2,
            2, 3, 0
        };
    };

    struct QuadSubmission
    {
        glm::vec2 Position{};
        float Rotation = 0.0f;
        glm::vec2 Size{1.0f};
        glm::vec4 Color{1.0f};
        Ref<Texture> TextureAsset = nullptr;
        int Layer = 0;
    };
}

#pragma once
#include <glm/glm.hpp>

namespace anv
{
	struct QuadVertex
	{
		glm::vec2 Position;
	};

    struct Quad
    {
        static inline QuadVertex vertices[4] =
        {
            {{-0.5f,-0.5f}},
            {{ 0.5f,-0.5f}},
            {{ 0.5f, 0.5f}},
            {{-0.5f, 0.5f}}
        };

        static inline uint32_t indices[6] =
        {
            0, 1, 2,
            2, 3, 0
        };
    };
}
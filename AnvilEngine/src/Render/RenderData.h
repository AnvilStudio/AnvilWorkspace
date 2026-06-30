#pragma once
#include <glm/glm.hpp>

namespace anv
{
	struct QuadVertex
	{
		glm::vec2 Position;
        glm::vec2 TexCoord;
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

	struct QuadSubmission
	{
		glm::vec2 Position;
        float Rotaion;
		glm::vec2 Size;
		glm::vec4 Color;

        int Layer;
	};
}
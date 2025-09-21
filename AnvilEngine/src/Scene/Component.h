#pragma once

#include <glm/glm.hpp>

namespace anv
{
    namespace Component
    {
        struct Transform
        {
            glm::vec3 Position{0.0f, 0.0f, 0.0f};
            glm::vec3 Rotation{0.0f, 0.0f, 0.0f};
            glm::vec3 Scale{1.0f, 1.0f, 1.0f};
        };
        
    } // namespace Component
    
} // namespace anv

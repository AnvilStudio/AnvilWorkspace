#pragma once
#include "../Core/Reference.h"
#include "../Core/Uuid.h"
#include "../Render/Camera.h"
#include <glm/glm.hpp>

namespace anv
{
    namespace Component
    {
        struct Tag
        {
            std::string tag = "";
            std::string Get() { return tag; }
        };

        struct UID
        {
            uuid::EntityUUID uid;
            UID()
            {
                uid = uuid::uuid_GenEntID();
            }
        };

        struct Transform
        {
            glm::vec3 Position{0.0f, 0.0f, 0.0f};
            glm::vec3 Rotation{0.0f, 0.0f, 0.0f};
            glm::vec3 Scale{1.0f, 1.0f, 1.0f};
        };

        struct Transform2d
        {
            glm::vec2 Position{ 0.0f, 0.0f};
            glm::vec2 Rotation{ 0.0f, 0.0f};
            glm::vec2 Scale{ 1.0f, 1.0f};
        };
        
    } // namespace Component
    
} // namespace anv

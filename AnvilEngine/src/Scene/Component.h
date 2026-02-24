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
            std::string Get() const { return tag; }
        };

        struct UID
        {
            uuid::EntityUUID uid;
            UID()
            {
                uid = uuid::uuid_GenEntID();
            }
        };

        //struct Transform3d
        //{
        //    glm::vec3 Position{0.0f};
        //    glm::vec3 Rotation{0.0f};
        //    glm::vec3 Scale{1.0f};
        //};

        struct Transform2d
        {
            glm::vec2 Position{0.0f};
            glm::vec2 Rotation{0.0f};
            glm::vec2 Scale{1.0f};

            Transform2d()
            {

            }

            Transform2d(glm::vec2 _pos, glm::vec2 _rot, glm::vec2 _scale)
                : Position(_pos), Rotation(_rot), Scale(_scale)
            {
            }
        };

        struct SpriteRenderer
        {
            uuid::AssetUUID Texture;
            glm::vec4 color{ 1, 1, 1, 1 };
            int layer;
        };
        
    } // namespace Component
    
} // namespace anv

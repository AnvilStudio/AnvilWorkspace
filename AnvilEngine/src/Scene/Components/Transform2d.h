#pragma once

#include "../../Util/Serialize/Serializer.h"

#include <glm/glm.hpp>

namespace anv::Component
{
    /**
     * @brief Position, rotation, and scale for a two-dimensional object.
     *
     * Kept in a standalone header so rendering types such as Camera2D can use
     * transforms without including the complete scene component collection.
     */
    struct Transform2d
    {
        glm::vec2 position{0.0f};
        glm::vec2 scale{1.0f};
        float rotation = 0.0f;

        Transform2d() = default;

        Transform2d(glm::vec2 _position, float _rotation, glm::vec2 _scale)
            : position(_position),
              scale(_scale),
              rotation(_rotation)
        {
        }

        void Serialize(Serializer& _serializer)
        {
            _serializer.Field("PositionX", position.x);
            _serializer.Field("PositionY", position.y);
            _serializer.Field("ScaleX", scale.x);
            _serializer.Field("ScaleY", scale.y);
            _serializer.Field("Rotation", rotation);
        }

        void Deserialize(Serializer& _serializer)
        {
            _serializer.Field("PositionX", position.x);
            _serializer.Field("PositionY", position.y);
            _serializer.Field("ScaleX", scale.x);
            _serializer.Field("ScaleY", scale.y);
            _serializer.Field("Rotation", rotation);
        }
    };
}

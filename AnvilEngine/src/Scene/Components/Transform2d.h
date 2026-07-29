#pragma once

#include "../../Util/Serialize/Serializer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

        glm::mat4 GetTransform() const
        {
            glm::mat4 transform{1.0f};

            transform = glm::translate(
                transform,
                glm::vec3(position, 0.0f));

            transform = glm::rotate(
                transform,
                glm::radians(rotation),
                glm::vec3(0.0f, 0.0f, 1.0f));

            transform = glm::scale(
                transform,
                glm::vec3(scale, 1.0f));

            return transform;
        }

        void SetFromMatrix(const glm::mat4 &matrix)
        {
            // Translation
            position.x = matrix[3][0];
            position.y = matrix[3][1];

            // Scale
            scale.x = glm::length(glm::vec2(matrix[0]));
            scale.y = glm::length(glm::vec2(matrix[1]));

            // Rotation (about Z)
            rotation = glm::degrees(
                std::atan2(matrix[0][1], matrix[0][0]));
        }

        void Serialize(Serializer &_serializer)
        {
            _serializer.Field("PositionX", position.x);
            _serializer.Field("PositionY", position.y);
            _serializer.Field("ScaleX", scale.x);
            _serializer.Field("ScaleY", scale.y);
            _serializer.Field("Rotation", rotation);
        }

        void Deserialize(Serializer &_serializer)
        {
            _serializer.Field("PositionX", position.x);
            _serializer.Field("PositionY", position.y);
            _serializer.Field("ScaleX", scale.x);
            _serializer.Field("ScaleY", scale.y);
            _serializer.Field("Rotation", rotation);
        }
    };
}

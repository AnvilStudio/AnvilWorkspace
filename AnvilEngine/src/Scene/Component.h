#pragma once
#include "../Core/Reference.h"
#include "../Core/Uuid.h"
#include "../Render/Camera.h"
#include "../Util/Serialize/Serializer.h"
#include "../vendor/entt/single_include/entt/entt.hpp"
#include <glm/glm.hpp>

namespace anv
{
    //=== Component Serialization ===

    template<typename T>
    concept SerializableComponent =
        requires(T t, Serializer & ser)
    {
        t.Serialize(ser);
    };

    template<typename T>
    concept DeserializableComponent =
        requires(T t, Serializer & ser)
    {
        t.Deserialize(ser);
    };

    template<SerializableComponent T>
    void SerializeIfPresent(
        entt::registry& registry,
        entt::entity entity,
        Serializer& ser,
        const std::string& name)
    {
        if (!registry.any_of<T>(entity))
            return;

        ser.Object(name, [&]()
        {
            registry.get<T>(entity).Serialize(ser);
        });
    }

    template<DeserializableComponent T>
    void DeserializeIfPresent(
        entt::registry& registry,
        entt::entity entity,
        Serializer& ser,
        const std::string& name)
    {
        if (!ser.HasObject(name))
            return;

        T& component = registry.any_of<T>(entity)
            ? registry.get<T>(entity)
            : registry.emplace<T>(entity);

        ser.Object(name, [&]()
        {
            component.Deserialize(ser);
        });
    }

    // ========================

    namespace Component
    {     
        struct Tag
        {
            std::string value = "";
            std::string Get() const { return value; }

            void Serialize(Serializer& _ser)
            {
                _ser.Field("Value", value);
            }

            void Deserialize(Serializer& _ser)
            {
                _ser.Field("Value", value);
            }
        };

        struct UID
        {
            uuid::EntityUUID uid;
            UID()
            {
                uid = uuid::uuid_GenEntID();
            }
        };

        struct Transform2d
        {
            glm::vec2 position{0.0f};
            glm::vec2 scale{1.0f};
            float rotation = 0.f;

            Transform2d()
            {

            }

            Transform2d(glm::vec2 _pos, float _rot, glm::vec2 _scale)
                : position(_pos), rotation(_rot), scale(_scale)
            {
            }

            void Serialize(Serializer& _ser)
            {
                _ser.Field("PositionX", position.x);
                _ser.Field("PositionY", position.y);

                _ser.Field("ScaleX", scale.x);
                _ser.Field("ScaleY", scale.y);

                _ser.Field("Rotation", rotation);
            }

            void Deserialize(Serializer& _ser)
            {
                _ser.Field("PositionX", position.x);
                _ser.Field("PositionY", position.y);

                _ser.Field("ScaleX", scale.x);
                _ser.Field("ScaleY", scale.y);

                _ser.Field("Rotation", rotation);
            }
        };

        struct SpriteRenderer
        {
            uuid::AssetUUID texture{};
            glm::vec4 color{ 1, 1, 1, 1 };
            int drawLayer;

            void Serialize(Serializer& _ser)
            {
                _ser.Field("TextureID", texture.uuid);
                _ser.Field("ColorR", color.x);
                _ser.Field("ColorG", color.y);
                _ser.Field("ColorB", color.z);
                _ser.Field("ColorA", color.w);
                _ser.Field("DrawLayer", drawLayer);
            }

            void Deserialize(Serializer& _ser)
            {
                _ser.Field("TextureID", texture.uuid);
                _ser.Field("ColorR", color.x);
                _ser.Field("ColorG", color.y);
                _ser.Field("ColorB", color.z);
                _ser.Field("ColorA", color.w);
                _ser.Field("DrawLayer", drawLayer);
            }
        };

    } // namespace Component
    
} // namespace anv

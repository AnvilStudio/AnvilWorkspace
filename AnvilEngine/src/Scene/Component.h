#pragma once

#include "../Core/Reference.h"
#include "../Core/Uuid.h"
#include "Components/Transform2d.h"
#include "../Render/Camera.h"
#include "../Scripting/ScriptTypes.h"
#include "../Util/Serialize/Serializer.h"
#include "../vendor/entt/single_include/entt/entt.hpp"

#include <glm/glm.hpp>

namespace anv
{
    template<typename T>
    concept SerializableComponent = requires(T t, Serializer& ser)
    {
        t.Serialize(ser);
    };

    template<typename T>
    concept DeserializableComponent = requires(T t, Serializer& ser)
    {
        t.Deserialize(ser);
    };

    template<SerializableComponent T>
    void SerializeIfPresent(entt::registry& registry, entt::entity entity,
                            Serializer& ser, const std::string& name)
    {
        if (!registry.any_of<T>(entity))
            return;

        ser.Object(name, [&]() { registry.get<T>(entity).Serialize(ser); });
    }

    template<DeserializableComponent T>
    void DeserializeIfPresent(entt::registry& registry, entt::entity entity,
                              Serializer& ser, const std::string& name)
    {
        if (!ser.HasObject(name))
            return;

        T& component = registry.any_of<T>(entity)
            ? registry.get<T>(entity)
            : registry.emplace<T>(entity);

        ser.Object(name, [&]() { component.Deserialize(ser); });
    }

    namespace Component
    {
        struct Tag
        {
            std::string value;
            std::string Get() const { return value; }

            void Serialize(Serializer& ser) { ser.Field("Value", value); }
            void Deserialize(Serializer& ser) { ser.Field("Value", value); }
        };

        struct UID
        {
            uuid::EntityUUID uid;
            UID() : uid(uuid::uuid_GenEntID()) {}
        };

        struct Camera2D
        {
            _shared<anv::Camera2D> camera = std::make_shared<anv::Camera2D>();
            bool isActive = false;

            void Serialize(Serializer& ser)
            {
                float zoom = camera ? camera->GetZoom() : 1.0f;
                ser.Field("IsActive", isActive);
                ser.Field("Zoom", zoom);
            }

            void Deserialize(Serializer& ser)
            {
                float zoom = 1.0f;
                ser.FieldOr<bool>("IsActive", isActive, false);
                ser.FieldOr<float>("Zoom", zoom, 1.0f);

                if (!camera)
                    camera = std::make_shared<anv::Camera2D>();

                camera->SetZoom(zoom);
            }
        };

        struct SpriteRenderer
        {
            uuid::AssetUUID texture{};
            glm::vec4 color{1.0f};
            int drawLayer = 0;

            void Serialize(Serializer& ser)
            {
                ser.Field("TextureID", texture.uuid);
                ser.Field("ColorR", color.x);
                ser.Field("ColorG", color.y);
                ser.Field("ColorB", color.z);
                ser.Field("ColorA", color.w);
                ser.Field("DrawLayer", drawLayer);
            }

            void Deserialize(Serializer& ser)
            {
                ser.Field("TextureID", texture.uuid);
                ser.Field("ColorR", color.x);
                ser.Field("ColorG", color.y);
                ser.Field("ColorB", color.z);
                ser.Field("ColorA", color.w);
                ser.Field("DrawLayer", drawLayer);
            }
        };

        enum class Rigidbody2DType
        {
            Static = 0,
            Kinematic = 1,
            Dynamic = 2
        };

        inline const char* Rigidbody2DTypeToString(Rigidbody2DType type)
        {
            switch (type)
            {
                case Rigidbody2DType::Static: return "Static";
                case Rigidbody2DType::Kinematic: return "Kinematic";
                case Rigidbody2DType::Dynamic: return "Dynamic";
            }
            return "Dynamic";
        }

        inline bool Rigidbody2DTypeFromString(
            const std::string& value,
            Rigidbody2DType& type)
        {
            if (value == "Static") { type = Rigidbody2DType::Static; return true; }
            if (value == "Kinematic") { type = Rigidbody2DType::Kinematic; return true; }
            if (value == "Dynamic") { type = Rigidbody2DType::Dynamic; return true; }
            return false;
        }

        struct Rigidbody2D
        {
            Rigidbody2DType type = Rigidbody2DType::Dynamic;
            float gravityScale = 1.0f;
            float linearDamping = 0.0f;
            float angularDamping = 0.0f;
            bool fixedRotation = false;
            bool bullet = false;
            bool enabled = true;

            void Serialize(Serializer& ser)
            {
                ser.EnumFieldOr("Type", type, Rigidbody2DType::Dynamic,
                    Rigidbody2DTypeToString, Rigidbody2DTypeFromString);
                ser.Field("GravityScale", gravityScale);
                ser.Field("LinearDamping", linearDamping);
                ser.Field("AngularDamping", angularDamping);
                ser.Field("FixedRotation", fixedRotation);
                ser.Field("Bullet", bullet);
                ser.Field("Enabled", enabled);
            }

            void Deserialize(Serializer& ser)
            {
                ser.EnumFieldOr("Type", type, Rigidbody2DType::Dynamic,
                    Rigidbody2DTypeToString, Rigidbody2DTypeFromString);
                ser.FieldOr<float>("GravityScale", gravityScale, 1.0f);
                ser.FieldOr<float>("LinearDamping", linearDamping, 0.0f);
                ser.FieldOr<float>("AngularDamping", angularDamping, 0.0f);
                ser.FieldOr<bool>("FixedRotation", fixedRotation, false);
                ser.FieldOr<bool>("Bullet", bullet, false);
                ser.FieldOr<bool>("Enabled", enabled, true);
            }
        };

        struct BoxCollider2D
        {
            glm::vec2 offset{0.0f};
            glm::vec2 size{1.0f};
            float density = 1.0f;
            float friction = 0.5f;
            float restitution = 0.0f;
            bool sensor = false;

            void Serialize(Serializer& ser)
            {
                ser.Field("OffsetX", offset.x);
                ser.Field("OffsetY", offset.y);
                ser.Field("SizeX", size.x);
                ser.Field("SizeY", size.y);
                ser.Field("Density", density);
                ser.Field("Friction", friction);
                ser.Field("Restitution", restitution);
                ser.Field("Sensor", sensor);
            }

            void Deserialize(Serializer& ser)
            {
                ser.FieldOr<float>("OffsetX", offset.x, 0.0f);
                ser.FieldOr<float>("OffsetY", offset.y, 0.0f);
                ser.FieldOr<float>("SizeX", size.x, 1.0f);
                ser.FieldOr<float>("SizeY", size.y, 1.0f);
                ser.FieldOr<float>("Density", density, 1.0f);
                ser.FieldOr<float>("Friction", friction, 0.5f);
                ser.FieldOr<float>("Restitution", restitution, 0.0f);
                ser.FieldOr<bool>("Sensor", sensor, false);
            }
        };

        struct Script
        {
            std::string modulePath;
            std::string className;
            bool enabled = true;
            ScriptFieldMap fields;

            void Serialize(Serializer& ser)
            {
                ser.Field("Module", modulePath);
                ser.Field("Class", className);
                ser.Field("Enabled", enabled);
                for (auto& [name, field] : fields)
                {
                    ser.ObjectKeyed("Fields", name, [&]() { field.Serialize(ser); });
                }
            }

            void Deserialize(Serializer& ser)
            {
                ser.FieldOr<std::string>("Module", modulePath, "");
                ser.FieldOr<std::string>("Class", className, "");
                ser.FieldOr<bool>("Enabled", enabled, true);
                fields.clear();
                ser.ForEachTable("Fields", [&](const std::string& name)
                {
                    ScriptField field;
                    field.Deserialize(ser);
                    fields.emplace(name, std::move(field));
                });
            }
        };
    }
}

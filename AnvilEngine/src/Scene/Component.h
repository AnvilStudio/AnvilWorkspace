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
    template <typename T>
    concept SerializableComponent = requires(T t, Serializer &ser) {
        t.Serialize(ser);
    };

    template <typename T>
    concept DeserializableComponent = requires(T t, Serializer &ser) {
        t.Deserialize(ser);
    };

    template <SerializableComponent T>
    void SerializeIfPresent(entt::registry &registry, entt::entity entity,
                            Serializer &ser, const std::string &name)
    {
        if (!registry.any_of<T>(entity))
            return;

        ser.Object(name, [&]()
                   { registry.get<T>(entity).Serialize(ser); });
    }

    template <DeserializableComponent T>
    void DeserializeIfPresent(entt::registry &registry, entt::entity entity,
                              Serializer &ser, const std::string &name)
    {
        if (!ser.HasObject(name))
            return;

        T &component = registry.any_of<T>(entity)
                           ? registry.get<T>(entity)
                           : registry.emplace<T>(entity);

        ser.Object(name, [&]()
                   { component.Deserialize(ser); });
    }

    namespace Component
    {
        struct Tag
        {
            std::string value;
            std::string Get() const { return value; }

            void Serialize(Serializer &ser) { ser.Field("Value", value); }
            void Deserialize(Serializer &ser) { ser.Field("Value", value); }
        };

        // Persistent parent reference; a missing component denotes a root entity.
        // Entity UUIDs remain stable across save/load unlike entt handles.
        struct Relationship
        {
            std::string parentUUID;

            void Serialize(Serializer &ser) { ser.Field("Parent", parentUUID); }
            void Deserialize(Serializer &ser) { ser.FieldOr<std::string>("Parent", parentUUID, ""); }
        };

        // Root-only reference to a reusable prefab asset.
        struct PrefabInstance
        {
            std::string sourcePath;
            void Serialize(Serializer& ser) { ser.Field("Source", sourcePath); }
            void Deserialize(Serializer& ser) { ser.FieldOr<std::string>("Source", sourcePath, ""); }
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

            void Serialize(Serializer &ser)
            {
                float zoom = camera ? camera->GetZoom() : 1.0f;
                ser.Field("IsActive", isActive);
                ser.Field("Zoom", zoom);
            }

            void Deserialize(Serializer &ser)
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
            bool visibleInGame = true;
            bool visibleInEditor = true;
            bool flipX = false;
            bool flipY = false;
            glm::vec2 uvMin{0.0f, 0.0f};
            glm::vec2 uvMax{1.0f, 1.0f};

            void Serialize(Serializer &ser)
            {
                ser.Field("TextureID", texture.uuid);

                ser.Field("ColorR", color.x);
                ser.Field("ColorG", color.y);
                ser.Field("ColorB", color.z);
                ser.Field("ColorA", color.w);

                ser.Field("DrawLayer", drawLayer);

                ser.Field("VisibleInGame", visibleInGame);
                ser.Field("VisibleInEditor", visibleInEditor);
                ser.Field("FlipX", flipX);
                ser.Field("FlipY", flipY);

                ser.Field("UVMinX", uvMin.x);
                ser.Field("UVMinY", uvMin.y);

                ser.Field("UVMaxX", uvMax.x);
                ser.Field("UVMaxY", uvMax.y);
            }

            void Deserialize(Serializer &ser)
            {
                ser.FieldOr<std::string>("TextureID", texture.uuid, "");

                ser.FieldOr<float>("ColorR", color.x, 1.0f);
                ser.FieldOr<float>("ColorG", color.y, 1.0f);
                ser.FieldOr<float>("ColorB", color.z, 1.0f);
                ser.FieldOr<float>("ColorA", color.w, 1.0f);

                ser.FieldOr<int>("DrawLayer", drawLayer, 0);

                ser.FieldOr<bool>("VisibleInGame", visibleInGame, true);
                ser.FieldOr<bool>("VisibleInEditor", visibleInEditor, true);
                ser.FieldOr<bool>("FlipX", flipX, false);
                ser.FieldOr<bool>("FlipY", flipY, false);

                ser.FieldOr<float>("UVMinX", uvMin.x, 0.f);
                ser.FieldOr<float>("UVMinY", uvMin.y, 0.f);

                ser.FieldOr<float>("UVMaxX", uvMax.x, 1.f);
                ser.FieldOr<float>("UVMaxY", uvMax.y, 1.f);
            }
        };

        struct Animator2D
        {
            struct State
            {
                std::string name;
                std::string clip;
                bool loop = true;
            };

            // Legacy clip remains supported for scenes without named states.
            std::string clipPath;
            std::vector<State> states;
            std::string defaultState;
            std::string currentState;
            bool autoplay = true;
            bool enabled = true;

            bool playing = false;
            size_t frame = 0;
            float elapsed = 0.0f;
            std::string loadedPath;

            std::string ActiveClip() const
            {
                if (!states.empty())
                {
                    for (const auto& state : states)
                        if (state.name == currentState) return state.clip;
                    return {};
                }
                return clipPath;
            }

            bool PlayState(const std::string& name)
            {
                for (const auto& state : states)
                    if (state.name == name && !state.clip.empty())
                    {
                        if (currentState != name)
                        {
                            currentState = name;
                            loadedPath.clear();
                            frame = 0;
                            elapsed = 0.0f;
                        }
                        playing = true;
                        return true;
                    }
                return false;
            }

            void Play() { playing = true; }
            void Pause() { playing = false; }
            void Stop()
            {
                playing = false;
                frame = 0;
                elapsed = 0.0f;
            }
            void Serialize(Serializer& ser)
            {
                ser.Field("Clip", clipPath);
                ser.Field("Autoplay", autoplay);
                ser.Field("Enabled", enabled);
                ser.Field("DefaultState", defaultState);
                for (size_t i = 0; i < states.size(); ++i)
                {
                    const auto& state = states[i];
                    const std::string n = std::to_string(i);
                    const std::string key = std::string(n.size() < 8 ? 8-n.size() : 0, '0') + n;
                    ser.ObjectKeyed("States", key, [&]
                    {
                        std::string name = state.name, clip = state.clip;
                        ser.Field("Name", name);
                        ser.Field("Clip", clip);
                        bool loop = state.loop;
                        ser.Field("Loop", loop);
                    });
                }
            }
            void Deserialize(Serializer& ser)
            {
                ser.FieldOr<std::string>("Clip", clipPath, "");
                ser.FieldOr<bool>("Autoplay", autoplay, true);
                ser.FieldOr<bool>("Enabled", enabled, true);
                ser.FieldOr<std::string>("DefaultState", defaultState, "");
                states.clear();
                ser.ForEachTable("States", [&](const std::string&)
                {
                    State state;
                    ser.FieldOr<std::string>("Name", state.name, "");
                    ser.FieldOr<std::string>("Clip", state.clip, "");
                    ser.FieldOr<bool>("Loop", state.loop, true);
                    if (!state.name.empty()) states.push_back(std::move(state));
                });
                currentState = defaultState;
                playing = false;
                frame = 0;
                elapsed = 0.0f;
                loadedPath.clear();
            }
        };

        enum class Rigidbody2DType
        {
            Static = 0,
            Kinematic = 1,
            Dynamic = 2
        };

        inline const char *Rigidbody2DTypeToString(Rigidbody2DType type)
        {
            switch (type)
            {
            case Rigidbody2DType::Static:
                return "Static";
            case Rigidbody2DType::Kinematic:
                return "Kinematic";
            case Rigidbody2DType::Dynamic:
                return "Dynamic";
            }
            return "Dynamic";
        }

        inline bool Rigidbody2DTypeFromString(
            const std::string &value,
            Rigidbody2DType &type)
        {
            if (value == "Static")
            {
                type = Rigidbody2DType::Static;
                return true;
            }
            if (value == "Kinematic")
            {
                type = Rigidbody2DType::Kinematic;
                return true;
            }
            if (value == "Dynamic")
            {
                type = Rigidbody2DType::Dynamic;
                return true;
            }
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

            void Serialize(Serializer &ser)
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

            void Deserialize(Serializer &ser)
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

            void Serialize(Serializer &ser)
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

            void Deserialize(Serializer &ser)
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

            void Serialize(Serializer &ser)
            {
                ser.Field("Module", modulePath);
                ser.Field("Class", className);
                ser.Field("Enabled", enabled);
                for (auto &[name, field] : fields)
                {
                    ser.ObjectKeyed("Fields", name, [&]()
                                    { field.Serialize(ser); });
                }
            }

            void Deserialize(Serializer &ser)
            {
                ser.FieldOr<std::string>("Module", modulePath, "");
                ser.FieldOr<std::string>("Class", className, "");
                ser.FieldOr<bool>("Enabled", enabled, true);
                fields.clear();
                ser.ForEachTable("Fields", [&](const std::string &name)
                                 {
                    ScriptField field;
                    field.Deserialize(ser);
                    fields.emplace(name, std::move(field)); });
            }
        };
    }
}

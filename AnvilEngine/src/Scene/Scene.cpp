#include "Scene.h"
#include "Component.h"

#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <Render/CameraController.h>
#include <Render/Renderer.h>
#include <Scripting/PythonScriptEngine.h>
#include <Util/Serialize/Serializer.h>

namespace anv
{
    Scene::Scene(std::string _name)
        : m_Name(_name)
    {
        ANV_LOG_INFO("Creating scene: %s", m_Name.c_str())
        m_UUID = uuid::uuid_GenAssetID();
        Init();
    }

    Scene::Scene(std::filesystem::path _path)
        : m_Path(_path.string())
    {
        m_Name = _path.filename().string();

        ANV_LOG_INFO("Creating scene: %s", m_Name.c_str())
        m_UUID = uuid::uuid_GenAssetID();
        Init();
    }

    Scene::~Scene()
    {
        PythonScriptEngine::ShutdownScene(*this);
    }

    void Scene::Init()
    {
        m_MainCamera = std::make_shared<Camera2D>();
        m_CameraController = std::make_unique<CameraController>(
            App::GetInstance()->GetInputSystem(),
            m_MainCamera
        );

        Load();
    }

    void Scene::Load()
    {
        Serializer ser(
            m_Path,
            Serializer::Mode::SER_MODE_TOML,
            Serializer::Direction::Read
        );

        ser.Object("Scene", [&]()
        {
            ser.Field("Name", m_Name);
            ser.Field("UUID", m_UUID.uuid);
            ser.Field("Path", m_Path);

            ser.EnumFieldOr(
                "Context",
                m_Context,
                SceneContext::CTX_3D,
                SceneContextToString,
                SceneContextFromString
            );
        });

        ser.ObjectIf("Camera", [&]()
        {
            ser.Object("Transform", [&]()
            {
                m_MainCamera->GetTransform().Deserialize(ser);
            });

            ser.Object("Settings", [&]()
            {
                float aspectRatio = 0.0f;
                float zoom = 1.0f;
                ser.Field("AspectRatio", aspectRatio);
                ser.Field("Zoom", zoom);

                m_MainCamera->SetAspectRatio(aspectRatio);
                m_MainCamera->SetZoom(zoom);
            });
        });

        if (ser.HasObject("Entities"))
        {
            ser.Object("Entities", [&]()
            {
                ser.ForEachObject([&](const std::string& _entityUUID)
                {
                    entt::entity entity = m_Registry.create();
                    auto& id = m_Registry.emplace<uuid::EntityUUID>(entity);
                    id.uuid = _entityUUID;

                    DeserializeIfPresent<Component::Tag>(
                        m_Registry,
                        entity,
                        ser,
                        "Tag"
                    );

                    DeserializeIfPresent<Component::Transform2d>(
                        m_Registry,
                        entity,
                        ser,
                        "Transform2d"
                    );

                    DeserializeIfPresent<Component::SpriteRenderer>(
                        m_Registry,
                        entity,
                        ser,
                        "SpriteRenderer"
                    );

                    DeserializeIfPresent<Component::Script>(
                        m_Registry,
                        entity,
                        ser,
                        "Script"
                    );
                });
            });
        }

        ser.Close();
    }

    void Scene::Save()
    {
        Serializer ser(
            m_Path,
            Serializer::Mode::SER_MODE_TOML,
            Serializer::Direction::Write
        );

        ser.Object("Scene", [&]()
        {
            ser.Field("Name", m_Name);
            ser.Field("UUID", m_UUID.uuid);
            ser.Field("Path", m_Path);
            ser.EnumFieldOr(
                "Context",
                m_Context,
                SceneContext::CTX_3D,
                SceneContextToString,
                SceneContextFromString
            );
        });

        ser.Object("Camera", [&]()
        {
            ser.Object("Transform", [&]()
            {
                m_MainCamera->GetTransform().Serialize(ser);
            });

            ser.Object("Settings", [&]()
            {
                float aspectRatio = m_MainCamera->GetAspectRatio();
                float zoom = m_MainCamera->GetZoom();

                ser.Field("AspectRatio", aspectRatio);
                ser.Field("Zoom", zoom);
            });
        });

        ser.RemoveObject("Entities");
        auto view = m_Registry.view<uuid::EntityUUID, Component::Tag>();
        for (auto [entity, id, tag] : view.each())
        {
            ser.ObjectKeyed("Entities", id.uuid, [&]()
            {
                SerializeIfPresent<Component::Tag>(
                    m_Registry,
                    entity,
                    ser,
                    "Tag"
                );

                SerializeIfPresent<Component::Transform2d>(
                    m_Registry,
                    entity,
                    ser,
                    "Transform2d"
                );

                SerializeIfPresent<Component::SpriteRenderer>(
                    m_Registry,
                    entity,
                    ser,
                    "SpriteRenderer"
                );

                SerializeIfPresent<Component::Script>(
                    m_Registry,
                    entity,
                    ser,
                    "Script"
                );
            });
        }

        ser.Close();
    }

    void Scene::Shutdown()
    {
        if (m_HasShutdown)
            return;

        m_HasShutdown = true;
        PythonScriptEngine::ShutdownScene(*this);
        Save();
    }

    void Scene::OnUpdate(float _deltaTime)
    {
        if (m_CameraController)
            m_CameraController->Update(_deltaTime);

        PythonScriptEngine::UpdateScene(*this, _deltaTime);
    }

    void Scene::Render()
    {
        auto view = m_Registry.view<
            Component::Transform2d,
            Component::SpriteRenderer>();

        auto assetManager = App::GetInstance()->GetAssetManager();

        view.each([&](
            auto,
            Component::Transform2d& _transform,
            Component::SpriteRenderer& _sprite)
        {
            Ref<Texture> texture = assetManager
                ? assetManager->GetAs<Texture>(_sprite.texture)
                : nullptr;

            Renderer2D::DrawQuad(
                _transform.position,
                _transform.rotation,
                _transform.scale,
                _sprite.color,
                texture,
                _sprite.drawLayer
            );
        });
    }

    entt::entity Scene::CreateEntity(std::string _tag)
    {
        entt::entity entity = m_Registry.create();

        m_Registry.emplace<uuid::EntityUUID>(entity) = uuid::uuid_GenEntID();
        m_Registry.emplace<Component::Transform2d>(entity);
        m_Registry.emplace<Component::Tag>(entity, _tag);

        return entity;
    }

    entt::entity Scene::RegisterEntity(std::string _tag, uuid::EntityUUID _uuid)
    {
        entt::entity entity = m_Registry.create();
        m_Registry.emplace<Component::Tag>(entity, _tag);
        m_Registry.emplace<uuid::EntityUUID>(entity, _uuid);

        return entity;
    }

    void Scene::DestroyEntity(entt::entity _entity)
    {
        if (_entity == entt::null || !m_Registry.valid(_entity))
            return;

        PythonScriptEngine::DestroyEntity(*this, _entity);
        m_Registry.destroy(_entity);
    }
}
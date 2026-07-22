#include "Scene.h"
#include <Core/App.h>
#include <Util/Serialize/Serializer.h>
#include <Render/CameraController.h>
#include <Render/Renderer.h>
#include <Asset/AssetTypes/Texture.h>

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
        //Shutdown();
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
        Serializer ser(m_Path, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Read);
         
        // scene header
        ser.Object("Scene", [&]() {
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

        // Camera
        ser.ObjectIf("Camera", [&]() {
            ser.Object("Transform", [&]() {
                m_MainCamera->GetTransform().Deserialize(ser);
            });
            ser.Object("Settings", [&]() {
                float ar, z;
                ser.Field<float>("AspectRatio", ar);
                ser.Field<float>("Zoom", z);

                m_MainCamera->SetZoom(z);
                });
            });

        // entities
        if (ser.HasObject("Entities"))
        {
            ser.Object("Entities", [&]()
                {
                    ser.ForEachObject([&](const std::string& entityUUID)
                        {
                            entt::entity entity = m_Registry.create();

                            auto& id =
                                m_Registry.emplace<uuid::EntityUUID>(entity);

                            id.uuid = entityUUID;

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
                        });
                });
        }
        
        ser.Close();
    }

    void Scene::Save()
    {
        Serializer ser(m_Path, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Write);

        // Header
        ser.Object("Scene", [&] {
            ser.Field("Name", m_Name);
            ser.Field("UUID", m_UUID.uuid);
            ser.Field("Path", m_Path);
            ser.EnumFieldOr(
                "Context",
                m_Context,
                SceneContext::CTX_3D,
                SceneContextToString,
                SceneContextFromString);
            });

        // Camera
        ser.Object("Camera", [&]() {
            ser.Object("Transform", [&]() {
                m_MainCamera->GetTransform().Serialize(ser);
                });
            ser.Object("Settings", [&]() {
                float aspectRatio = m_MainCamera->GetAspectRatio();
                float zoom = m_MainCamera->GetZoom();

                ser.Field("AspectRatio", aspectRatio);
                ser.Field("Zoom", zoom);
            });
        });

        // Entities
        ser.RemoveObject("Entities");
        auto view = m_Registry.view<uuid::EntityUUID, Component::Tag>();
        for (auto [e, id, tag] : view.each())
        {
            ser.ObjectKeyed("Entities", id.uuid, [&] {

                SerializeIfPresent<Component::Tag>(
                    m_Registry,
                    e,
                    ser,
                    "Tag"
                );
                SerializeIfPresent<Component::Transform2d>(
                    m_Registry,
                    e,
                    ser,
                    "Transform2d"
                );
                SerializeIfPresent<Component::SpriteRenderer>(
                    m_Registry,
                    e,
                    ser,
                    "SpriteRenderer"
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
        Save();
    }

    void Scene::OnUpdate(float _deltaTime)
    {
        m_CameraController->Update(_deltaTime);
    }

    void Scene::Render()
    {
        auto view = m_Registry.view<
            Component::Transform2d,
            Component::SpriteRenderer>();

        auto assetManager = App::GetInstance()->GetAssetManager();

        view.each([&](
            auto entity,
            Component::Transform2d& transform,
            Component::SpriteRenderer& sprite)
            {
                Ref<Texture> texture = assetManager
                    ? assetManager->GetAs<Texture>(sprite.texture)
                    : nullptr;

                Renderer2D::DrawQuad(
                    transform.position,
                    transform.rotation,
                    transform.scale,
                    sprite.color,
                    texture,
                    sprite.drawLayer);
            });
    }

    entt::entity Scene::CreateEntity(std::string _tag)
    {
        auto entity = m_Registry.create();

        m_Registry.emplace<uuid::EntityUUID>(entity) = uuid::uuid_GenEntID();
        m_Registry.emplace<Component::Transform2d>(entity);
        m_Registry.emplace<Component::Tag>(entity, _tag);

        return entity;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (entity == entt::null)
            return;

        if (!m_Registry.valid(entity))
            return;

        m_Registry.destroy(entity);
    }       

    entt::entity Scene::RegisterEntity(std::string _tag, uuid::EntityUUID _uuid)
    {
        auto entity = m_Registry.create();
        m_Registry.emplace<Component::Tag>(entity, _tag);
        m_Registry.emplace<uuid::EntityUUID>(entity, _uuid);

        return entity;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (!m_Registry.valid(entity))
            return;

        m_Registry.destroy(entity);
    }
}
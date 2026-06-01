#include "Scene.h"
#include <Core/App.h>
#include <Util/Serialize/Serializer.h>
#include <Render/CameraController.h>
#include<Render/Renderer.h>

namespace anv
{
    Scene::Scene(std::string _name)
        : m_Name(_name)
    {
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
        m_Path = "Assets/Scenes/" + m_Name + ".ascn";

        m_MainCamera = std::make_shared<Camera2D>();
        m_CameraController = std::make_unique<CameraController>( 
            App::GetInstance()->GetInputSystem(), 
            m_MainCamera
        );
    }

    void Scene::Shutdown()
    {
        if (m_HasShutdown)
            return;

        m_HasShutdown = true;

        Serializer ser(m_Path, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Write);

        // Header
        ser.Object("Scene", [&] {
            ser.Field("Name", m_Name);
            ser.Field("UUID", m_UUID.uuid);
            ser.Field("Path", m_Path);
            ser.EnumFieldOr(
                "Context",
                m_Context,
                Scene::Context::CTX_3D,
                SceneContextToString,
                SceneContextFromString);

            // Entities
            // Could probably impl a Serialize component to gather all data of the entity and serialize it
            auto view = m_Registry.view<uuid::EntityUUID, Component::Tag>();
            for (auto [e, id, tag] : view.each())
            {
                ser.ObjectKeyed("Entities", id.uuid, [&] {
                    ser.Field("Name", tag.tag);
                });
            }
        });
        ser.Close();
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

        view.each([](
            auto entity,
            Component::Transform2d& transform,
            Component::SpriteRenderer& sprite)
            {
                Renderer2D::DrawQuad(
                    transform.Position,
                    transform.Scale,
                    sprite.Color
                );
            });
    }

    entt::entity Scene::CreateEntity(std::string _tag)
    {
        auto entity = m_Registry.create();

        m_Registry.emplace<uuid::EntityUUID>(entity);
        m_Registry.emplace<Component::Transform2d>(entity);
        m_Registry.emplace<Component::Tag>(entity, _tag);

        return entity;
    }

    entt::entity Scene::RegisterEntity(std::string _tag, uuid::EntityUUID _uuid)
    {
        auto entity = m_Registry.create();
        m_Registry.emplace<Component::Tag>(entity, _tag);
        m_Registry.emplace<uuid::EntityUUID>(entity, _uuid);

        return entity;
    }

    void Scene::DestroyEntity(entt::entity _ent)
    {
        m_Registry.destroy(_ent);
    }
}
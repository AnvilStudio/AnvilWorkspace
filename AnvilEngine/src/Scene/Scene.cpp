#include "Scene.h"
#include <Util/Serialize/Serializer.h>

namespace anv
{
    Scene::Scene(std::string _name)
        : m_Name(std::move(_name))
    {
        ANV_LOG_INFO("Creating scene: %s", m_Name.c_str())
        m_UUID = uuid::uuid_GenAssetID();
        Init();
    }

    void Scene::Init()
    {

    }

    void Scene::Shutdown()
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
                Scene::Context::CTX_3D,
                SceneContextToString,
                SceneContextFromString
            );
        });

        // Entities
        auto view = m_Registry.view<uuid::EntityUUID, Component::Tag>();
        for (auto [e, id, tag] : view.each())
        {
            ser.ObjectKeyed("Entities", id.uuid, [&] {
                ser.Field("Name", tag.tag);
            });
        }

        ser.Close();
    }

    void Scene::OnUpdate(float _deltaTime)
    {

    }

    void Scene::Render()
    {

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
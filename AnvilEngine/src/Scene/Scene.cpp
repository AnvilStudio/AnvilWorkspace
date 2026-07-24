#include "Scene.h"
#include "Component.h"
#include <Core/App.h>
#include <Util/Serialize/Serializer.h>
#include <Render/CameraController.h>
#include <Render/Renderer.h>
#include <Asset/AssetTypes/Texture.h>
#include <Scripting/PythonScriptEngine.h>

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
    }

    void Scene::Load()
    {
        Serializer serializer(m_Path);
        if (!serializer.Load())
        {
            ANV_LOG_ERROR("Failed to load scene: %s", m_Path.c_str())
            return;
        }

        serializer.Object("Scene", [&]()
        {
            serializer.Field("Name", m_Name);
            serializer.Field("UUID", m_UUID.uuid);

            serializer.ForEachTable("Entities", [&](const std::string& entityID)
            {
                serializer.Object(entityID, [&]()
                {
                    std::string tag;
                    serializer.Field("Tag", tag);

                    uuid::EntityUUID uuid;
                    uuid.uuid = entityID;
                    entt::entity entity = RegisterEntity(tag, uuid);

                    DeserializeIfPresent<Component::Tag>(
                        m_Registry,
                        entity,
                        serializer,
                        "TagComponent"
                    );

                    DeserializeIfPresent<Component::Transform2d>(
                        m_Registry,
                        entity,
                        serializer,
                        "Transform2dComponent"
                    );

                    DeserializeIfPresent<Component::SpriteRenderer>(
                        m_Registry,
                        entity,
                        serializer,
                        "SpriteRendererComponent"
                    );

                    DeserializeIfPresent<Component::Script>(
                        m_Registry,
                        entity,
                        serializer,
                        "ScriptComponent"
                    );
                });
            });
        });

        PythonScriptEngine::InitializeScene(*this);
    }

    void Scene::Save()
    {
        Serializer serializer(m_Path);

        serializer.Object("Scene", [&]()
        {
            serializer.Field("Name", m_Name);
            serializer.Field("UUID", m_UUID.uuid);

            auto view = m_Registry.view<uuid::EntityUUID, Component::Tag>();
            for (auto [entity, entityUUID, tag] : view.each())
            {
                serializer.ObjectKeyed("Entities", entityUUID.uuid, [&]()
                {
                    serializer.Field("Tag", tag.value);

                    SerializeIfPresent<Component::Tag>(
                        m_Registry,
                        entity,
                        serializer,
                        "TagComponent"
                    );

                    SerializeIfPresent<Component::Transform2d>(
                        m_Registry,
                        entity,
                        serializer,
                        "Transform2dComponent"
                    );

                    SerializeIfPresent<Component::SpriteRenderer>(
                        m_Registry,
                        entity,
                        serializer,
                        "SpriteRendererComponent"
                    );

                    SerializeIfPresent<Component::Script>(
                        m_Registry,
                        entity,
                        serializer,
                        "ScriptComponent"
                    );
                });
            }
        });

        if (!serializer.Save())
            ANV_LOG_ERROR("Failed to save scene: %s", m_Path.c_str())
    }

    void Scene::Shutdown()
    {
        PythonScriptEngine::ShutdownScene(*this);
        m_Registry.clear();
    }

    void Scene::OnUpdate(float _deltaTime)
    {
        if (m_CameraController)
            m_CameraController->OnUpdate(_deltaTime);

        PythonScriptEngine::UpdateScene(*this, _deltaTime);
    }

    void Scene::Render()
    {
        auto view = m_Registry.view<
            Component::Transform2d,
            Component::SpriteRenderer>();

        for (auto [entity, transform, sprite] : view.each())
        {
            Ref<Texture> texture = AssetManager::GetAsset<Texture>(sprite.texture);
            if (!texture)
                continue;

            Renderer::SubmitSprite(
                texture,
                transform.position,
                transform.scale,
                transform.rotation,
                sprite.color,
                sprite.drawLayer
            );
        }
    }

    entt::entity Scene::CreateEntity(std::string _tag)
    {
        entt::entity entity = m_Registry.create();
        m_Registry.emplace<uuid::EntityUUID>(entity, uuid::uuid_GenEntID());
        m_Registry.emplace<Component::Tag>(entity, _tag);
        m_Registry.emplace<Component::Transform2d>(entity);
        return entity;
    }

    entt::entity Scene::RegisterEntity(std::string _tag, uuid::EntityUUID _uuid)
    {
        entt::entity entity = m_Registry.create();
        m_Registry.emplace<uuid::EntityUUID>(entity, _uuid);
        m_Registry.emplace<Component::Tag>(entity, _tag);
        return entity;
    }

    void Scene::DestroyEntity(entt::entity _entity)
    {
        if (!m_Registry.valid(_entity))
            return;

        PythonScriptEngine::DestroyEntity(*this, _entity);
        m_Registry.destroy(_entity);
    }
}

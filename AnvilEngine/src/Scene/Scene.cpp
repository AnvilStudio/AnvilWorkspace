#include "Scene.h"
#include "Component.h"

#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <Physics/Physics2D.h>
#include <Render/CameraController.h>
#include <Render/Renderer.h>
#include <Scripting/PythonScriptEngine.h>
#include <Util/Serialize/Serializer.h>

namespace anv
{
    Scene::Scene(std::string name)
        : m_Name(std::move(name))
    {
        ANV_LOG_INFO("Creating scene: %s", m_Name.c_str())
        m_UUID = uuid::uuid_GenAssetID();
        Init();
    }

    Scene::Scene(std::filesystem::path path)
        : m_Path(path.string()),
          m_Name(path.filename().string())
    {
        ANV_LOG_INFO("Creating scene: %s", m_Name.c_str())
        m_UUID = uuid::uuid_GenAssetID();
        Init();
    }

    Scene::~Scene()
    {
        StopPhysics();
        PythonScriptEngine::ShutdownScene(*this);
    }

    void Scene::Init()
    {
        m_MainCamera = std::make_shared<Camera2D>();
        m_CameraController = std::make_unique<CameraController>(
            App::GetInstance()->GetInputSystem(),
            m_MainCamera);

        if (m_Path.empty())
            return;

        std::error_code error;
        const std::filesystem::path scenePath(m_Path);
        const bool exists = std::filesystem::exists(scenePath, error);
        if (error || !exists)
            return;

        error.clear();
        const auto fileSize = std::filesystem::file_size(scenePath, error);
        if (!error && fileSize > 0)
            Load();
    }

    void Scene::Load()
    {
        Serializer ser(m_Path, Serializer::Mode::SER_MODE_TOML,
                       Serializer::Direction::Read);

        ser.Object("Scene", [&]()
        {
            ser.Field("Name", m_Name);
            ser.Field("UUID", m_UUID.uuid);
            ser.Field("Path", m_Path);
            ser.EnumFieldOr("Context", m_Context, SceneContext::CTX_3D,
                            SceneContextToString, SceneContextFromString);
        });

        ser.ObjectIf("Camera", [&]()
        {
            ser.Object("Transform", [&]()
            {
                m_MainCamera->GetTransform().Deserialize(ser);
            });

            ser.Object("Settings", [&]()
            {
                float aspectRatio = 16.0f / 9.0f;
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
                ser.ForEachObject([&](const std::string& entityUUID)
                {
                    const entt::entity entity = m_Registry.create();
                    auto& id = m_Registry.emplace<uuid::EntityUUID>(entity);
                    id.uuid = entityUUID;

                    DeserializeIfPresent<Component::Tag>(m_Registry, entity, ser, "Tag");
                    DeserializeIfPresent<Component::Transform2d>(m_Registry, entity, ser, "Transform2d");
                    DeserializeIfPresent<Component::SpriteRenderer>(m_Registry, entity, ser, "SpriteRenderer");
                    DeserializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                    DeserializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                    DeserializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
                });
            });
        }

        ser.Close();
    }

    void Scene::Save()
    {
        if (m_Path.empty())
            return;

        Serializer ser(m_Path, Serializer::Mode::SER_MODE_TOML,
                       Serializer::Direction::Write);

        ser.Object("Scene", [&]()
        {
            ser.Field("Name", m_Name);
            ser.Field("UUID", m_UUID.uuid);
            ser.Field("Path", m_Path);
            ser.EnumFieldOr("Context", m_Context, SceneContext::CTX_3D,
                            SceneContextToString, SceneContextFromString);
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
            (void)tag;
            ser.ObjectKeyed("Entities", id.uuid, [&]()
            {
                SerializeIfPresent<Component::Tag>(m_Registry, entity, ser, "Tag");
                SerializeIfPresent<Component::Transform2d>(m_Registry, entity, ser, "Transform2d");
                SerializeIfPresent<Component::SpriteRenderer>(m_Registry, entity, ser, "SpriteRenderer");
                SerializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                SerializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                SerializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
            });
        }

        ser.Close();
    }

    void Scene::Shutdown()
    {
        if (m_HasShutdown)
            return;

        m_HasShutdown = true;
        StopPhysics();
        PythonScriptEngine::ShutdownScene(*this);
    }

    void Scene::SetScriptExecutionEnabled(bool enabled)
    {
        if (m_ScriptExecutionEnabled == enabled)
            return;

        m_ScriptExecutionEnabled = enabled;
        if (!enabled)
            PythonScriptEngine::ShutdownScene(*this);
    }

    void Scene::StartPhysics()
    {
        if (!m_Physics2D)
            m_Physics2D = std::make_unique<Physics2D>();

        if (!m_Physics2D->IsRunning())
            m_Physics2D->Start(*this);
    }

    void Scene::StopPhysics()
    {
        if (m_Physics2D)
            m_Physics2D->Stop();
    }

    bool Scene::IsPhysicsRunning() const
    {
        return m_Physics2D && m_Physics2D->IsRunning();
    }

    void Scene::OnUpdate(float deltaTime)
    {
        if (m_CameraController)
            m_CameraController->Update(deltaTime);

        if (m_ScriptExecutionEnabled)
            PythonScriptEngine::UpdateScene(*this, deltaTime);

        if (m_Physics2D && m_Physics2D->IsRunning())
            m_Physics2D->Step(*this, deltaTime);
    }

    void Scene::Render()
    {
        auto view = m_Registry.view<Component::Transform2d, Component::SpriteRenderer>();
        auto assetManager = App::GetInstance()->GetAssetManager();

        view.each([&](auto, Component::Transform2d& transform,
                      Component::SpriteRenderer& sprite)
        {
            Ref<Texture> texture = assetManager
                ? assetManager->GetAs<Texture>(sprite.texture)
                : nullptr;

            Renderer2D::DrawQuad(transform.position, transform.rotation,
                transform.scale, sprite.color, texture, sprite.drawLayer);
        });
    }

    entt::entity Scene::CreateEntity(std::string tag)
    {
        const entt::entity entity = m_Registry.create();
        m_Registry.emplace<uuid::EntityUUID>(entity) = uuid::uuid_GenEntID();
        m_Registry.emplace<Component::Transform2d>(entity);
        m_Registry.emplace<Component::Tag>(entity, std::move(tag));
        return entity;
    }

    entt::entity Scene::RegisterEntity(std::string tag, uuid::EntityUUID id)
    {
        const entt::entity entity = m_Registry.create();
        m_Registry.emplace<Component::Tag>(entity, std::move(tag));
        m_Registry.emplace<uuid::EntityUUID>(entity, std::move(id));
        return entity;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (entity == entt::null || !m_Registry.valid(entity))
            return;

        if (m_Physics2D)
            m_Physics2D->DestroyBody(entity);

        PythonScriptEngine::DestroyEntity(*this, entity);
        m_Registry.destroy(entity);
    }
}

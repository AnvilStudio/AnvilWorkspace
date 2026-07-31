#include "Scene.h"
#include "Component.h"

#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <Physics/Physics2D.h>
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

        bool hasLegacyCamera = false;
        Component::Transform2d legacyCameraTransform{};
        float legacyCameraZoom = 1.0f;

        ser.ObjectIf("Camera", [&]()
        {
            hasLegacyCamera = true;

            ser.Object("Transform", [&]()
            {
                legacyCameraTransform.Deserialize(ser);
            });

            ser.Object("Settings", [&]()
            {
                float unusedAspectRatio = 16.0f / 9.0f;
                ser.FieldOr<float>("AspectRatio", unusedAspectRatio, 16.0f / 9.0f);
                ser.FieldOr<float>("Zoom", legacyCameraZoom, 1.0f);
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
                    DeserializeIfPresent<Component::Camera2D>(m_Registry, entity, ser, "Camera2D");
                    DeserializeIfPresent<Component::SpriteRenderer>(m_Registry, entity, ser, "SpriteRenderer");
                    DeserializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                    DeserializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                    DeserializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
                });
            });
        }

        ser.Close();

        if (hasLegacyCamera && GetActiveCameraEntity() == entt::null)
        {
            const entt::entity cameraEntity = CreateEntity("Game Camera");
            GetComponent<Component::Transform2d>(cameraEntity) = legacyCameraTransform;

            auto& cameraComponent = AddComponent<Component::Camera2D>(cameraEntity);
            cameraComponent.isActive = true;
            cameraComponent.camera->SetZoom(legacyCameraZoom);

            ANV_LOG_INFO("Migrated legacy scene camera to Game Camera entity.");
        }
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

        ser.RemoveObject("Camera");
        ser.RemoveObject("Entities");

        auto view = m_Registry.view<uuid::EntityUUID, Component::Tag>();
        for (auto [entity, id, tag] : view.each())
        {
            (void)tag;
            ser.ObjectKeyed("Entities", id.uuid, [&]()
            {
                SerializeIfPresent<Component::Tag>(m_Registry, entity, ser, "Tag");
                SerializeIfPresent<Component::Transform2d>(m_Registry, entity, ser, "Transform2d");
                SerializeIfPresent<Component::Camera2D>(m_Registry, entity, ser, "Camera2D");
                SerializeIfPresent<Component::SpriteRenderer>(m_Registry, entity, ser, "SpriteRenderer");
                SerializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                SerializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                SerializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
            });
        }

        ser.Close();
    }

    _shared<Camera2D> Scene::GetActiveCamera()
    {
        auto view = m_Registry.view<Component::Transform2d, Component::Camera2D>();

        for (const auto entity : view)
        {
            auto& transform = view.get<Component::Transform2d>(entity);
            auto& cameraComponent = view.get<Component::Camera2D>(entity);

            if (!cameraComponent.isActive)
                continue;

            if (!cameraComponent.camera)
                cameraComponent.camera = std::make_shared<Camera2D>();

            cameraComponent.camera->SetTransform(transform);
            cameraComponent.camera->Update(0.0f);
            return cameraComponent.camera;
        }

        return nullptr;
    }

    entt::entity Scene::GetActiveCameraEntity() const
    {
        auto view = m_Registry.view<Component::Camera2D>();

        for (const auto entity : view)
        {
            if (view.get<Component::Camera2D>(entity).isActive)
                return entity;
        }

        return entt::null;
    }

    void Scene::SetActiveCamera(entt::entity entity)
    {
        if (entity == entt::null ||
            !m_Registry.valid(entity) ||
            !m_Registry.any_of<Component::Camera2D>(entity))
        {
            return;
        }

        auto view = m_Registry.view<Component::Camera2D>();
        for (const auto cameraEntity : view)
        {
            view.get<Component::Camera2D>(cameraEntity).isActive =
                cameraEntity == entity;
        }
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

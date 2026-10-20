#include "Scene.h"
#include "Component.h"

#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <Physics/PhysicsSystem2D.h>
#include <Render/Renderer.h>
#include <Scripting/PythonScriptEngine.h>
#include <Util/Serialize/Serializer.h>

#include <limits>
#include <unordered_set>
#include <cmath>

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
        Component::Transform2D legacyCameraTransform{};
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
                    DeserializeIfPresent<Component::Transform2D>(m_Registry, entity, ser, "Transform2d");
                    DeserializeIfPresent<Component::Camera2D>(m_Registry, entity, ser, "Camera2D");
                    DeserializeIfPresent<Component::SpriteRenderer>(m_Registry, entity, ser, "SpriteRenderer");
                    DeserializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                    DeserializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                    DeserializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
                    DeserializeIfPresent<Component::Relationship>(m_Registry, entity, ser, "Relationship");
                });
            });
        }

        ser.Close();

        if (hasLegacyCamera && GetActiveCameraEntity() == entt::null)
        {
            const entt::entity cameraEntity = CreateEntity("Game Camera");
            GetComponent<Component::Transform2D>(cameraEntity) = legacyCameraTransform;

            auto& cameraComponent = AddComponent<Component::Camera2D>(cameraEntity);
            cameraComponent.isActive = true;
            cameraComponent.camera->SetZoom(legacyCameraZoom);

            ANV_LOG_INFO("Migrated legacy scene camera to Game Camera entity.");
        }

        auto cameraView = m_Registry.view<Component::Camera2D>();
        for (const auto entity : cameraView)
            EnsureCameraEditorSprite(entity);
    }

    void Scene::Save()
    {
        if (m_Path.empty())
            return;

        auto cameraView = m_Registry.view<Component::Camera2D>();
        for (const auto entity : cameraView)
            EnsureCameraEditorSprite(entity);

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
                SerializeIfPresent<Component::Transform2D>(m_Registry, entity, ser, "Transform2d");
                SerializeIfPresent<Component::Camera2D>(m_Registry, entity, ser, "Camera2D");
                SerializeIfPresent<Component::SpriteRenderer>(m_Registry, entity, ser, "SpriteRenderer");
                SerializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                SerializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                SerializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
                SerializeIfPresent<Component::Relationship>(m_Registry, entity, ser, "Relationship");
            });
        }

        ser.Close();
    }

    _shared<Camera2D> Scene::GetActiveCamera()
    {
        auto view = m_Registry.view<Component::Transform2D, Component::Camera2D>();

        for (const auto entity : view)
        {
            auto& transform = view.get<Component::Transform2D>(entity);
            auto& cameraComponent = view.get<Component::Camera2D>(entity);

            if (!cameraComponent.isActive)
                continue;

            EnsureCameraEditorSprite(entity);

            if (!cameraComponent.camera)
                cameraComponent.camera = std::make_shared<Camera2D>();

            Component::Transform2D worldTransform;
            worldTransform.SetFromMatrix(GetWorldTransform(entity));
            cameraComponent.camera->SetTransform(worldTransform);
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

        EnsureCameraEditorSprite(entity);

        auto view = m_Registry.view<Component::Camera2D>();
        for (const auto cameraEntity : view)
        {
            view.get<Component::Camera2D>(cameraEntity).isActive =
                cameraEntity == entity;
        }
    }

    void Scene::EnsureCameraEditorSprite(entt::entity entity)
    {
        if (entity == entt::null ||
            !m_Registry.valid(entity) ||
            !m_Registry.any_of<Component::Camera2D>(entity))
        {
            return;
        }

        auto& sprite = AddComponent<Component::SpriteRenderer>(entity);
        sprite.visibleInGame = false;
        sprite.visibleInEditor = true;
        sprite.drawLayer = std::numeric_limits<int>::max();
        sprite.color = glm::vec4(1.0f);

        if (!sprite.texture.uuid.empty())
            return;

        auto assetManager = App::GetInstance()->GetAssetManager();
        if (!assetManager)
            return;

        const std::filesystem::path iconPath =
            App::GetInstance()->GetFS().GetKeyVal("Res") /
            "icons" /
            "camera_icon.png";

        std::error_code error;
        if (!std::filesystem::is_regular_file(iconPath, error))
        {
            ANV_LOG_WARN(
                "Camera editor icon was not found at '%s'.",
                iconPath.string().c_str());
            return;
        }

        Ref<Texture> icon = assetManager->GetOrCreateTexture(iconPath);
        if (!icon)
        {
            ANV_LOG_WARN(
                "Failed to create camera editor icon texture from '%s'.",
                iconPath.string().c_str());
            return;
        }

        sprite.texture = icon->GetAssetID();
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
        if (!m_PhysicsSystem2D)
            m_PhysicsSystem2D = std::make_unique<PhysicsSystem2D>();

        if (!m_PhysicsSystem2D->IsRunning())
            m_PhysicsSystem2D->Start(*this);
    }

    void Scene::StopPhysics()
    {
        if (m_PhysicsSystem2D)
            m_PhysicsSystem2D->Stop();
    }

    bool Scene::IsPhysicsRunning() const
    {
        return m_PhysicsSystem2D && m_PhysicsSystem2D->IsRunning();
    }

    void Scene::OnUpdate(float deltaTime)
    {
        if (m_ScriptExecutionEnabled)
            PythonScriptEngine::UpdateScene(*this, deltaTime);

        if (m_PhysicsSystem2D && m_PhysicsSystem2D->IsRunning())
            m_PhysicsSystem2D->Step(*this, deltaTime);
    }

    void Scene::Render()
    {
        auto view = m_Registry.view<Component::Transform2D, Component::SpriteRenderer>();
        auto assetManager = App::GetInstance()->GetAssetManager();

        view.each([&](auto entity, Component::Transform2D& transform,
                      Component::SpriteRenderer& sprite)
        {
            if (!sprite.visibleInGame)
                return;

            Ref<Texture> texture = assetManager
                ? assetManager->GetAs<Texture>(sprite.texture)
                : nullptr;

            Component::Transform2D worldTransform;
            worldTransform.SetFromMatrix(GetWorldTransform(entity));
            Renderer2D::DrawQuad(worldTransform.position, worldTransform.rotation,
                worldTransform.scale, sprite.color, texture, sprite.drawLayer);
        });
    }

    entt::entity Scene::CreateEntity(std::string tag)
    {
        const entt::entity entity = m_Registry.create();
        m_Registry.emplace<uuid::EntityUUID>(entity) = uuid::uuid_GenEntID();
        m_Registry.emplace<Component::Transform2D>(entity);
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


    entt::entity Scene::GetParent(entt::entity entity) const
    {
        if (!m_Registry.valid(entity) || !m_Registry.any_of<Component::Relationship>(entity))
            return entt::null;

        const auto& id = m_Registry.get<Component::Relationship>(entity).parentUUID;
        if (id.empty())
            return entt::null;

        auto view = m_Registry.view<uuid::EntityUUID>();
        for (auto candidate : view)
            if (view.get<uuid::EntityUUID>(candidate).uuid == id)
                return candidate;
        return entt::null;
    }

    std::vector<entt::entity> Scene::GetChildren(entt::entity entity) const
    {
        std::vector<entt::entity> children;
        if (!m_Registry.valid(entity) || !m_Registry.any_of<uuid::EntityUUID>(entity))
            return children;

        const auto& id = m_Registry.get<uuid::EntityUUID>(entity).uuid;
        auto view = m_Registry.view<Component::Relationship>();
        for (auto candidate : view)
            if (view.get<Component::Relationship>(candidate).parentUUID == id)
                children.push_back(candidate);
        return children;
    }

    bool Scene::IsDescendant(entt::entity entity, entt::entity ancestor) const
    {
        // An acyclic scene is enforced by SetParent; bound iteration also
        // protects against corrupt external/serialized relationships.
        const auto limit = m_Registry.storage<entt::entity>().size();
        for (std::size_t i = 0; i < limit &&
             entity != entt::null && m_Registry.valid(entity); ++i)
        {
            if (entity == ancestor)
                return true;
            entity = GetParent(entity);
        }
        return false;
    }

    glm::mat4 Scene::GetWorldTransform(entt::entity entity) const
    {
        glm::mat4 result(1.0f);
        // Pre-multiply ancestors: parent * child, no temporary allocations.
        // The bound also prevents endless traversal of malformed relationships.
        const auto limit = m_Registry.storage<entt::entity>().size();
        for (std::size_t i = 0; i < limit &&
             entity != entt::null && m_Registry.valid(entity); ++i)
        {
            if (m_Registry.any_of<Component::Transform2D>(entity))
                result = m_Registry.get<Component::Transform2D>(entity).GetTransform() * result;
            entity = GetParent(entity);
        }
        return result;
    }

    bool Scene::SetWorldTransform(entt::entity entity, const glm::mat4& world)
    {
        if (!HasComponent<Component::Transform2D>(entity))
            return false;

        const auto parent = GetParent(entity);
        const glm::mat4 parentWorld = parent == entt::null
            ? glm::mat4(1.0f) : GetWorldTransform(parent);
        if (std::abs(glm::determinant(parentWorld)) < 0.000001f)
            return false;

        GetComponent<Component::Transform2D>(entity).SetFromMatrix(
            glm::inverse(parentWorld) * world);
        return true;
    }

    bool Scene::SetParent(entt::entity child, entt::entity parent, bool keepWorld)
    {
        if (!m_Registry.valid(child) || !HasComponent<uuid::EntityUUID>(child) ||
            (parent != entt::null && (!m_Registry.valid(parent) ||
                                       !HasComponent<uuid::EntityUUID>(parent))) ||
            child == parent ||
            (parent != entt::null && IsDescendant(parent, child)))
            return false;

        if (GetParent(child) == parent)
            return true;

        const bool preserve = keepWorld && HasComponent<Component::Transform2D>(child);
        const glm::mat4 world = preserve
            ? GetWorldTransform(child) : glm::mat4(1.0f);
        const glm::mat4 parentWorld = preserve && parent != entt::null
            ? GetWorldTransform(parent) : glm::mat4(1.0f);
        if (preserve &&
            std::abs(glm::determinant(parentWorld)) < 0.000001f)
            return false;

        if (parent == entt::null)
            m_Registry.remove<Component::Relationship>(child);
        else
            m_Registry.emplace_or_replace<Component::Relationship>(
                child, m_Registry.get<uuid::EntityUUID>(parent).uuid);

        if (preserve)
            SetWorldTransform(child, world);
        return true;
    }

    void Scene::DestroyEntity(entt::entity entity)
    {
        if (entity == entt::null || !m_Registry.valid(entity))
            return;

        // Orphan children without jumping to a new world position.
        for (auto child : GetChildren(entity))
            SetParent(child, entt::null, true);

        if (m_PhysicsSystem2D)
            m_PhysicsSystem2D->DestroyBody(entity);

        PythonScriptEngine::DestroyEntity(*this, entity);
        m_Registry.destroy(entity);
    }
}

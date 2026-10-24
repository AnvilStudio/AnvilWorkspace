#include "Scene.h"
#include "Component.h"

#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <Physics/PhysicsSystem2D.h>
#include <Render/Renderer.h>
#include <Scripting/PythonScriptEngine.h>
#include <Util/Serialize/Serializer.h>
#include <Util/FileSys/FileSystem.h>

#include <limits>
#include <chrono>
#include <unordered_set>
#include <cmath>
#include <algorithm>

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
                     DeserializeIfPresent<Component::Animator2D>(m_Registry, entity, ser, "Animator2D");
                    DeserializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                    DeserializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                    DeserializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
                    DeserializeIfPresent<Component::Relationship>(m_Registry, entity, ser, "Relationship");
                    DeserializeIfPresent<Component::PrefabInstance>(m_Registry, entity, ser, "PrefabInstance");
                });
            });
        }

        ser.Close();
        m_SnapshotReady = false;

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
                 SerializeIfPresent<Component::Animator2D>(m_Registry, entity, ser, "Animator2D");
                SerializeIfPresent<Component::Rigidbody2D>(m_Registry, entity, ser, "Rigidbody2D");
                SerializeIfPresent<Component::BoxCollider2D>(m_Registry, entity, ser, "BoxCollider2D");
                SerializeIfPresent<Component::Script>(m_Registry, entity, ser, "Script");
                SerializeIfPresent<Component::Relationship>(m_Registry, entity, ser, "Relationship");
                SerializeIfPresent<Component::PrefabInstance>(m_Registry, entity, ser, "PrefabInstance");
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

        std::filesystem::path iconPath =
            App::GetInstance()->GetFS().GetKeyVal("Res") /
            "Icons" / "camera_icon.png";

        std::error_code error;
        if (!std::filesystem::is_regular_file(iconPath, error))
        {
            // Engine resources are copied alongside Forge/AnvRuntime.
            iconPath = FileSystem::s_ExecDir / "Anvil" / "Resources" /
                       "Icons" / "camera_icon.png";
            error.clear();
        }
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
        // Rebuild ancestry before physics/script updates. Any transform changes
        // during the update are handled by the subsequent render snapshot.
        RebuildWorldTransformSnapshot();
        if (m_ScriptExecutionEnabled)
        {
            ANV_PROFILE_ZONE(Script);
            PythonScriptEngine::UpdateScene(*this, deltaTime);
        }

        if (m_PhysicsSystem2D && m_PhysicsSystem2D->IsRunning())
            m_PhysicsSystem2D->Step(*this, deltaTime);

        if (m_ScriptExecutionEnabled)
            UpdateAnimations(deltaTime);
    }

    void Scene::UpdateAnimations(float deltaTime)
    {
        if (!(deltaTime > 0.0f) || !std::isfinite(deltaTime)) return;
        auto view = m_Registry.view<Component::Animator2D, Component::SpriteRenderer>();
        for (auto entity : view)
        {
            auto& animator = view.get<Component::Animator2D>(entity);
            auto& sprite = view.get<Component::SpriteRenderer>(entity);
            if (!animator.enabled) continue;
            if (!animator.states.empty() && animator.currentState.empty())
                animator.currentState = animator.defaultState.empty()
                    ? animator.states.front().name : animator.defaultState;
            const std::string activePath = animator.ActiveClip();
            if (activePath.empty()) continue;

            if (animator.loadedPath != activePath)
            {
                animator.loadedPath = activePath;
                animator.frame = 0;
                animator.elapsed = 0.0f;
                animator.playing = animator.autoplay || animator.playing;
            }

            auto it = m_AnimationClips.find(activePath);
            if (it == m_AnimationClips.end())
            {
                SpriteAnimationClip clip;
                std::filesystem::path path(activePath);
                if (path.is_relative())
                    path = std::filesystem::path(m_Path).parent_path() / path;
                if (!clip.Load(path)) continue;
                it = m_AnimationClips.emplace(activePath, std::move(clip)).first;
            }

            const auto& clip = it->second;
            bool shouldLoop = clip.loop;
            if (!animator.states.empty())
                for (const auto& state : animator.states)
                    if (state.name == animator.currentState)
                    {
                        shouldLoop = state.loop;
                        break;
                    }
            if (clip.frames.empty()) continue;
            if (!clip.textureUUID.empty())
                sprite.texture.uuid = clip.textureUUID;
            animator.frame = std::min(animator.frame, clip.frames.size() - 1);
            if (animator.playing)
            {
                animator.elapsed += deltaTime;
                while (animator.elapsed >= clip.frames[animator.frame].duration)
                {
                    animator.elapsed -= clip.frames[animator.frame].duration;
                    if (animator.frame + 1 < clip.frames.size())
                        ++animator.frame;
                    else if (shouldLoop)
                        animator.frame = 0;
                    else
                    {
                        animator.playing = false;
                        animator.elapsed = 0.0f;
                        break;
                    }
                }
            }
            if (!clip.frames[animator.frame].textureUUID.empty())
                sprite.texture.uuid = clip.frames[animator.frame].textureUUID;
            sprite.uvMin = clip.frames[animator.frame].uvMin;
            sprite.uvMax = clip.frames[animator.frame].uvMax;
        }
    }

    void Scene::Render()
    {
        ANV_PROFILE_ZONE(SceneRender);
        RebuildWorldTransformSnapshot();
        auto view = m_Registry.view<Component::Transform2D, Component::SpriteRenderer>();
        auto assetManager = App::GetInstance()->GetAssetManager();

        struct SpriteItem
        {
            entt::entity entity;
            Component::Transform2D pose;
            Ref<Texture> texture;
            glm::vec4 color;
            int drawLayer;
            glm::vec2 uvMin;
            glm::vec2 uvMax;
        };
        static thread_local std::vector<SpriteItem> items;
        items.clear();
        items.reserve(m_Registry.storage<Component::SpriteRenderer>().size());

        // Phase 1: gather sprite data and resolve texture assets.
        {
            ANV_PROFILE_ZONE(SpriteLookup);
            for (auto entity : view)
            {
                const auto& sprite = view.get<Component::SpriteRenderer>(entity);
                if (!sprite.visibleInGame) continue;
                Ref<Texture> texture = assetManager && !sprite.texture.uuid.empty()
                    ? assetManager->GetAs<Texture>(sprite.texture) : nullptr;
                items.push_back({entity, {}, texture, sprite.color, sprite.drawLayer,
                    {sprite.flipX ? sprite.uvMax.x : sprite.uvMin.x,
                     sprite.flipY ? sprite.uvMax.y : sprite.uvMin.y},
                    {sprite.flipX ? sprite.uvMin.x : sprite.uvMax.x,
                     sprite.flipY ? sprite.uvMin.y : sprite.uvMax.y}});
            }
        }

        // Phase 2: resolve each sprite's world pose once, with shared-parent caching.
        {
            ANV_PROFILE_ZONE(SpriteTransform);
            for (auto& item : items)
            {
                if (m_Registry.any_of<Component::Relationship>(item.entity))
                    item.pose = GetSnapshotWorldPose2D(item.entity);
                else
                    item.pose = m_Registry.get<Component::Transform2D>(item.entity);
            }
        }

        // Phase 3: submit quads; clocks are sampled per phase, not per entity.
        {
            ANV_PROFILE_ZONE(SpriteSubmit);
            for (const auto& item : items)
                Renderer2D::DrawQuad(item.pose.position, item.pose.rotation,
                    item.pose.scale, item.color, item.texture, item.drawLayer,
                    item.uvMin, item.uvMax);
        }
    }

    entt::entity Scene::CreateEntity(std::string tag)
    {
        const entt::entity entity = m_Registry.create();
        m_Registry.emplace<uuid::EntityUUID>(entity) = uuid::uuid_GenEntID();
        m_Registry.emplace<Component::Transform2D>(entity);
        m_Registry.emplace<Component::Tag>(entity, std::move(tag));
        m_SnapshotReady = false;
        return entity;
    }

    entt::entity Scene::RegisterEntity(std::string tag, uuid::EntityUUID id)
    {
        const entt::entity entity = m_Registry.create();
        m_Registry.emplace<Component::Tag>(entity, std::move(tag));
        m_Registry.emplace<uuid::EntityUUID>(entity, std::move(id));
        m_SnapshotReady = false;
        return entity;
    }


    entt::entity Scene::GetParent(entt::entity entity) const
    {
        if (!m_Registry.valid(entity) || !m_Registry.any_of<Component::Relationship>(entity))
            return entt::null;

        const auto& id = m_Registry.get<Component::Relationship>(entity).parentUUID;
        if (id.empty())
            return entt::null;

        if (m_SnapshotReady)
        {
            const auto it = m_ParentIndex.find(entity);
            return it == m_ParentIndex.end() ? entt::null : it->second;
        }
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
        const auto limit = m_Registry.storage<entt::entity>()->size();
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
        const auto limit = m_Registry.storage<entt::entity>()->size();
        for (std::size_t i = 0; i < limit &&
             entity != entt::null && m_Registry.valid(entity); ++i)
        {
            if (m_Registry.any_of<Component::Transform2D>(entity))
                result = m_Registry.get<Component::Transform2D>(entity).GetTransform() * result;
            entity = GetParent(entity);
        }
        return result;
    }

    void Scene::RebuildWorldTransformSnapshot() const
    {
        m_UUIDIndex.clear();
        m_ParentIndex.clear();
        m_WorldSnapshot.clear();

        auto ids = m_Registry.view<uuid::EntityUUID>();
        if (const auto* storage = m_Registry.storage<uuid::EntityUUID>())
            m_UUIDIndex.reserve(storage->size());
        for (auto entity : ids)
            m_UUIDIndex.emplace(ids.get<uuid::EntityUUID>(entity).uuid, entity);

        auto relationships = m_Registry.view<Component::Relationship>();
        if (const auto* storage = m_Registry.storage<Component::Relationship>())
            m_ParentIndex.reserve(storage->size());
        for (auto entity : relationships)
        {
            const auto& id = relationships.get<Component::Relationship>(entity).parentUUID;
            auto parent = m_UUIDIndex.find(id);
            if (parent != m_UUIDIndex.end() && parent->second != entity)
                m_ParentIndex.emplace(entity, parent->second);
        }
        m_SnapshotReady = true;
    }

    glm::mat4 Scene::GetSnapshotWorldTransform(entt::entity entity) const
    {
        GetSnapshotWorldPose2D(entity);
        auto it = m_WorldSnapshot.find(entity);
        return it == m_WorldSnapshot.end() ? glm::mat4(1.0f) : it->second.matrix;
    }

    Component::Transform2D Scene::GetSnapshotWorldPose2D(entt::entity entity) const
    {
        if (!m_SnapshotReady)
            RebuildWorldTransformSnapshot();

        if (auto found = m_WorldSnapshot.find(entity); found != m_WorldSnapshot.end())
            return found->second.pose;
        if (!m_Registry.valid(entity))
            return {};

        std::vector<entt::entity> chain;
        chain.reserve(8);
        entt::entity cursor = entity;
        const std::size_t maxDepth = m_Registry.storage<entt::entity>()->size();
        for (std::size_t i = 0; i < maxDepth && cursor != entt::null; ++i)
        {
            if (m_WorldSnapshot.contains(cursor))
                break;
            if (std::find(chain.begin(), chain.end(), cursor) != chain.end())
                break;
            chain.push_back(cursor);
            const auto it = m_ParentIndex.find(cursor);
            cursor = it == m_ParentIndex.end() ? entt::null : it->second;
        }

        WorldSnapshotEntry parent{};
        if (auto known = m_WorldSnapshot.find(cursor); known != m_WorldSnapshot.end())
            parent = known->second;

        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        {
            if (!m_Registry.any_of<Component::Transform2D>(*it))
                continue;

            const auto& local = m_Registry.get<Component::Transform2D>(*it);
            WorldSnapshotEntry current;
            const bool hasParent = m_ParentIndex.contains(*it);
            if (!hasParent)
            {
                current.pose = local;
                current.matrix = local.GetTransform();
                current.uniformScale = std::abs(local.scale.x - local.scale.y) < 0.00001f;
            }
            else if (parent.uniformScale && parent.pose.scale.x > 0.0f &&
                     std::abs(local.scale.x - local.scale.y) < 0.00001f)
            {
                // Uniform scale + rotation preserves ordinary TRS composition.
                const float angle = glm::radians(parent.pose.rotation);
                const float c = std::cos(angle), s = std::sin(angle);
                const glm::vec2 scaled = local.position * parent.pose.scale;
                current.pose.position = parent.pose.position +
                    glm::vec2(c * scaled.x - s * scaled.y,
                              s * scaled.x + c * scaled.y);
                current.pose.rotation = parent.pose.rotation + local.rotation;
                current.pose.scale = parent.pose.scale * local.scale;
                current.uniformScale = true;
                current.matrix = current.pose.GetTransform();
            }
            else
            {
                // Mixed nonuniform scale and rotation may produce shear.
                current.matrix = parent.matrix * local.GetTransform();
                current.pose.SetFromMatrix(current.matrix);
                current.uniformScale = false;
            }
            parent = current;
            m_WorldSnapshot.insert_or_assign(*it, current);
        }
        auto result = m_WorldSnapshot.find(entity);
        return result != m_WorldSnapshot.end() ? result->second.pose : Component::Transform2D{};
    }

    bool Scene::SetWorldTransform(entt::entity entity, const glm::mat4& world)
    {
        if (!HasComponent<Component::Transform2D>(entity))
            return false;

        auto& transform = GetComponent<Component::Transform2D>(entity);
        // Root entities are already in world space.
        if (!HasComponent<Component::Relationship>(entity) || GetParent(entity) == entt::null)
        {
            transform.SetFromMatrix(world);
            return true;
        }

        const glm::mat4 parentWorld = GetWorldTransform(GetParent(entity));
        if (std::abs(glm::determinant(parentWorld)) < 0.000001f)
            return false;
        transform.SetFromMatrix(glm::inverse(parentWorld) * world);
        return true;
    }

    bool Scene::SetWorldPose2D(entt::entity entity, glm::vec2 position, float rotationDegrees)
    {
        if (!HasComponent<Component::Transform2D>(entity))
            return false;

        auto& local = GetComponent<Component::Transform2D>(entity);
        if (!m_Registry.any_of<Component::Relationship>(entity))
        {
            local.position = position;
            local.rotation = rotationDegrees;
            return true;
        }
        const entt::entity parent = GetParent(entity);
        if (parent == entt::null)
        {
            local.position = position;
            local.rotation = rotationDegrees;
            return true;
        }

        const glm::mat4 parentWorld = GetWorldTransform(parent);
        if (std::abs(glm::determinant(parentWorld)) < 0.000001f)
            return false;

        // Keep the current local scale; the physics body controls only pose.
        const glm::mat4 inverseParent = glm::inverse(parentWorld);
        const glm::vec4 localPosition = inverseParent * glm::vec4(position, 0.0f, 1.0f);
        local.position = glm::vec2(localPosition);
        // The direction basis accounts for ancestor rotations without extra
        // world-transform decomposition of the child itself.
        const glm::vec2 xAxis = glm::vec2(inverseParent * glm::vec4(
            std::cos(glm::radians(rotationDegrees)), std::sin(glm::radians(rotationDegrees)), 0.0f, 0.0f));
        local.rotation = glm::degrees(std::atan2(xAxis.y, xAxis.x));
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

        // Rebuilding immediately keeps public parent queries correct.
        m_SnapshotReady = false;
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
        m_SnapshotReady = false;
    }

    float Scene::GetPhysStepTime()
    {
        return m_PhysicsSystem2D->m_StepTimer; 
    }
}

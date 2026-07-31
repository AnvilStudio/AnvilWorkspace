#pragma once

#include "../vendor/entt/single_include/entt/entt.hpp"
#include "../Render/Camera.h"
#include "SceneData.h"

namespace anv
{
    class Physics2D;

    class Scene : public RefCounter
    {
    public:
        Scene(std::string name);
        Scene(std::filesystem::path path);
        ~Scene();

        void Init();
        void Load();
        void Save();
        void Save() const { const_cast<Scene*>(this)->Save(); }
        void Shutdown();
        void OnUpdate(float deltaTime);
        void Render();

        std::string GetName() { return m_Name; }
        uuid::AssetUUID GetUUID() { return m_UUID; }
        std::string GetPath() { return m_Path; }
        _shared<Camera2D> GetMainCamera() { return m_MainCamera; }
        entt::registry& Registry() { return m_Registry; }
        const entt::registry& Registry() const { return m_Registry; }

        void SetScriptExecutionEnabled(bool enabled);
        bool IsScriptExecutionEnabled() const { return m_ScriptExecutionEnabled; }

        void StartPhysics();
        void StopPhysics();
        bool IsPhysicsRunning() const;

        entt::entity CreateEntity(std::string tag);
        entt::entity RegisterEntity(std::string tag, uuid::EntityUUID uuid);

        template<typename ComponentType, typename... Args>
        ComponentType& AddComponent(entt::entity entity, Args&&... args)
        {
            if (m_Registry.any_of<ComponentType>(entity))
                return m_Registry.get<ComponentType>(entity);

            return m_Registry.emplace<ComponentType>(
                entity,
                std::forward<Args>(args)...);
        }

        template<typename ComponentType>
        ComponentType& GetComponent(entt::entity entity)
        {
            return m_Registry.get<ComponentType>(entity);
        }

        template<typename ComponentType>
        ComponentType& GetComponent(entt::entity entity) const
        {
            return const_cast<entt::registry&>(m_Registry)
                .get<ComponentType>(entity);
        }

        template<typename ComponentType>
        void RemoveComponent(entt::entity entity)
        {
            if (!m_Registry.valid(entity) ||
                !m_Registry.any_of<ComponentType>(entity))
            {
                return;
            }

            m_Registry.remove<ComponentType>(entity);
        }

        template<typename ComponentType>
        bool HasComponent(entt::entity entity) const
        {
            return m_Registry.valid(entity) &&
                m_Registry.any_of<ComponentType>(entity);
        }

        void DestroyEntity(entt::entity entity);

    protected:
        bool m_HasShutdown = false;
        bool m_ScriptExecutionEnabled = false;
        std::string m_Name;
        SceneContext m_Context{};
        uuid::AssetUUID m_UUID;
        entt::registry m_Registry;
        std::string m_Path;
        _shared<Camera2D> m_MainCamera = nullptr;
        _unique<Physics2D> m_Physics2D = nullptr;

        friend class SceneManager;
        friend class SceneRenderer2D;
    };

    inline const char* SceneContextToString(SceneContext context)
    {
        switch (context)
        {
            case SceneContext::CTX_2D: return "2D";
            case SceneContext::CTX_3D: return "3D";
            default: return "Unknown";
        }
    }

    inline bool SceneContextFromString(
        const std::string& value,
        SceneContext& context)
    {
        if (value == "2D" || value == "CTX_2D")
        {
            context = SceneContext::CTX_2D;
            return true;
        }

        if (value == "3D" || value == "CTX_3D")
        {
            context = SceneContext::CTX_3D;
            return true;
        }

        return false;
    }
}

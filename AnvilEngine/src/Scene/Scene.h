#pragma once
#include "../vendor/entt/single_include/entt/entt.hpp"
#include "../Render/Camera.h"
#include "../Render/CameraController.h"

namespace anv
{

    // probably need a scene ID?
    class Scene
        : public RefCounter
    {
    public:
        enum class Context
        {
            CTX_2D,
            CTX_3D
        };

        Scene(std::string _name);
        ~Scene();

        void Init();
        void Shutdown();
        void OnUpdate(float _deltaTime);
        void Render();

        std::string GetName() { return m_Name; }
        uuid::AssetUUID GetUUID() { return m_UUID; }
        std::string GetPath() { return m_Path; }
        _shared<Camera2D> GetMainCamera() { return m_MainCamera; }
        entt::registry& Registry() { return m_Registry; }


        entt::entity CreateEntity (std::string _tag);
        entt::entity RegisterEntity(std::string _tag, uuid::EntityUUID _uuid);
        
        template<typename comp, typename ...Args>
        comp& AddComponent(entt::entity _entity, Args&&... args)
        {
            return m_Registry.emplace<comp>(_entity, std::forward<Args>(args)...);
        }

        void DestroyEntity(entt::entity);

    
    protected:
        bool m_HasShutdown = false;
        std::string m_Name;
        Context     m_Context;
        uuid::AssetUUID m_UUID;
        entt::registry  m_Registry;
        std::string m_Path;
        _shared<Camera2D> m_MainCamera = nullptr;
        _unique<CameraController> m_CameraController = nullptr;

        
        friend class SceneManager;
        friend class SceneRenderer2D;
    };
   
    inline const char* SceneContextToString(Scene::Context ctx)
    {
        switch (ctx)
        {
        case Scene::Context::CTX_2D: return "2D";
        case Scene::Context::CTX_3D: return "3D";
        default: return "Unknown";
        }
    }

    inline bool SceneContextFromString(const std::string& s, Scene::Context& out)
    {
        if (s == "2D" || s == "CTX_2D") { out = Scene::Context::CTX_2D; return true; }
        if (s == "3D" || s == "CTX_3D") { out = Scene::Context::CTX_3D; return true; }
        return false;
    }
}
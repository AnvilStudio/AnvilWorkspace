#pragma once
#include "../vendor/entt/single_include/entt/entt.hpp"
#include "../Render/Camera.h"

namespace anv
{


    // probably need a scene ID?
    class Scene
    {
    public:
        Scene(std::string _name);

        virtual void Init()     = 0;
        virtual void Shutdown() = 0;
        virtual void OnUpdate(float _deltaTime) = 0;
        virtual void Render()   = 0;

        virtual entt::entity CreateEntity (std::string _tag) = 0;
        virtual void DestroyEntity(entt::entity) = 0;

    
    protected:
        std::string m_Name;
        uuid::AssetUUID m_UUID;
        entt::registry m_Registry;
    };
   

}
#include "Scene2d.h"
#include "../Component.h"
#include "Render/Renderer.h"

namespace anv
{
    Scene2D::Scene2D(std::string _name) :
        Scene(_name)
    {
        ANV_LOG_INFO("Instantiated scene: %s", _name)
        m_UUID = uuid::uuid_GenAssetID();
        Init();
    }

    void Scene2D::Init()     
    {
        // Find the path to the scenes folder, look for the scene file. if not found, create one
    }

    void Scene2D::Shutdown() 
    {
        // write the scenes data to the scene file.
    }

    void Scene2D::OnUpdate(float _deltaTime) 
    {
        
    }
    
    void Scene2D::Render()  
    {
        
    }

    entt::entity Scene2D::CreateEntity(std::string _tag) 
    {
        auto entity = m_Registry.create();

        m_Registry.emplace<Component::UID>(entity);
        m_Registry.emplace<Component::Transform2d>(entity);
        m_Registry.emplace<Component::Tag>(entity, _tag);
        
        return entity;
    }

    void Scene2D::DestroyEntity(entt::entity _ent) 
    {
        m_Registry.destroy(_ent);
    }
}
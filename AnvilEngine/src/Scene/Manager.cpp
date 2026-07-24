#include "Manager.h"
#include "Scene.h"
#include "Core/App.h"
#include "SceneData.h"

namespace anv
{
    SceneManager::SceneManager()
    {
        ANV_LOG_INFO("Initializing Scene Manager")
    }

    SceneManager::~SceneManager()
    {
        Shutdown();
    }

    Ref<Scene> SceneManager::GetActive()
    {
        auto it = m_Registry.find(m_Active);
        if (it == m_Registry.end())
            return nullptr;
        return it->second;
    }

    Ref<Scene> SceneManager::Create(std::string _name)
    {
        Ref<Scene> scene = Ref<Scene>::Create(_name);
        const uuid::AssetUUID id = scene->m_UUID;
        m_Registry.emplace(id, scene);

        // First scene becomes active by default.
        if (m_Active.uuid.empty())
            m_Active = id;

        return scene;
    }

    Ref<Scene> SceneManager::Register(std::string _path)
    {
        auto& fs = App::GetInstance()->GetFS();

        std::filesystem::path newPath;
        if (_path[0] == '@')
        {
            newPath = fs.ResolveKey(_path);
        }
        else
        {
            ANV_LOG_WARN("Scene path %s did not match with any file system mounts!", _path.c_str());
            newPath = std::filesystem::path(_path);
        }

        Ref<Scene> scene = Ref<Scene>::Create(newPath);
        m_Registry.emplace(scene->m_UUID, scene);

        // If no active scene, set it.
        if (m_Active.uuid.empty())
            m_Active = scene->GetUUID();

        return scene;
    }

    void SceneManager::Register(Ref<Scene> _scene)
    {
        m_Registry.emplace(_scene->GetUUID(), _scene);
    }

    void SceneManager::SetActive(uuid::AssetUUID _sceneUUID)
    {
        ANV_ASSERT(
            m_Registry.find(_sceneUUID) != m_Registry.end(),
            "Scene must be registered with the scene manager in order to be set as the active scene")

        m_Active = _sceneUUID;
    }

    Ref<Scene> SceneManager::ReloadActive()
    {
        Ref<Scene> current = GetActive();
        if (!current)
            return nullptr;

        const std::filesystem::path scenePath = current->GetPath();
        if (scenePath.empty())
        {
            ANV_LOG_ERROR("Cannot reload the active scene because it has no file path.");
            return nullptr;
        }

        current->SetScriptExecutionEnabled(false);

        const uuid::AssetUUID previousID = m_Active;
        Ref<Scene> reloaded = Ref<Scene>::Create(scenePath);
        if (!reloaded)
            return nullptr;

        m_Registry.erase(previousID);
        m_Registry[reloaded->GetUUID()] = reloaded;
        m_Active = reloaded->GetUUID();

        ANV_LOG_INFO("Reloaded active scene from '%s'.", scenePath.string().c_str());
        return reloaded;
    }

    void SceneManager::Shutdown()
    {
        if (m_HasShutdown)
            return;

        m_HasShutdown = true;

        for (auto& [uuid, scene] : m_Registry)
        {
            if (scene)
                scene->Shutdown();
        }

        m_Registry.clear();
    }
}

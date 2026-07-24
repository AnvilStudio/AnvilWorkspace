#include "Manager.h"
#include "Scene.h"
#include "Core/App.h"
#include "SceneData.h"

#include <system_error>

namespace
{
    std::filesystem::path NormalizeScenePath(const std::filesystem::path& _path)
    {
        if (_path.empty())
            return {};

        std::filesystem::path path = _path;
        const std::string pathString = path.string();
        if (!pathString.empty() && pathString.front() == '@')
            path = anv::App::GetInstance()->GetFS().ResolveKey(pathString);

        std::error_code error;
        std::filesystem::path normalized =
            std::filesystem::weakly_canonical(path, error);
        if (!error)
            return normalized;

        error.clear();
        normalized = std::filesystem::absolute(path, error);
        return error ? path : normalized;
    }
}

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
        const std::filesystem::path newPath = NormalizeScenePath(_path);
        if (newPath.empty())
        {
            ANV_LOG_ERROR("Cannot register a scene with an empty path.");
            return nullptr;
        }

        Ref<Scene> scene = Ref<Scene>::Create(newPath);
        m_Registry[scene->m_UUID] = scene;

        // If no active scene, set it.
        if (m_Active.uuid.empty())
            m_Active = scene->GetUUID();

        return scene;
    }

    void SceneManager::Register(Ref<Scene> _scene)
    {
        if (!_scene)
            return;

        m_Registry[_scene->GetUUID()] = _scene;
    }

    void SceneManager::SetActive(uuid::AssetUUID _sceneUUID)
    {
        ANV_ASSERT(
            m_Registry.find(_sceneUUID) != m_Registry.end(),
            "Scene must be registered with the scene manager in order to be set as the active scene")

        m_Active = _sceneUUID;
    }

    Ref<Scene> SceneManager::OpenScene(
        const std::filesystem::path& _path,
        bool _createIfMissing)
    {
        const std::filesystem::path scenePath = NormalizeScenePath(_path);
        if (scenePath.empty())
        {
            ANV_LOG_ERROR("Cannot open a scene with an empty path.");
            return nullptr;
        }

        if (scenePath.extension() != ".ascn")
        {
            ANV_LOG_ERROR("Scene '%s' does not use the .ascn extension.", scenePath.string().c_str());
            return nullptr;
        }

        std::error_code error;
        const bool exists = std::filesystem::exists(scenePath, error);
        if (error)
        {
            ANV_LOG_ERROR("Unable to inspect scene '%s': %s", scenePath.string().c_str(), error.message().c_str());
            return nullptr;
        }

        if (!exists && !_createIfMissing)
        {
            ANV_LOG_ERROR("Scene '%s' does not exist.", scenePath.string().c_str());
            return nullptr;
        }

        for (auto& [id, registeredScene] : m_Registry)
        {
            if (!registeredScene)
                continue;

            if (NormalizeScenePath(registeredScene->GetPath()) == scenePath)
            {
                Ref<Scene> current = GetActive();
                if (current && current != registeredScene)
                {
                    current->SetScriptExecutionEnabled(false);
                    current->Save();
                }

                m_Active = id;
                ANV_LOG_INFO("Activated scene '%s'.", scenePath.string().c_str());
                return registeredScene;
            }
        }

        Ref<Scene> current = GetActive();
        if (current)
        {
            current->SetScriptExecutionEnabled(false);
            current->Save();
        }

        if (!exists)
        {
            std::filesystem::create_directories(scenePath.parent_path(), error);
            if (error)
            {
                ANV_LOG_ERROR("Unable to create scene directory '%s': %s", scenePath.parent_path().string().c_str(), error.message().c_str());
                return nullptr;
            }
        }

        Ref<Scene> scene = Ref<Scene>::Create(scenePath);
        if (!scene)
            return nullptr;

        m_Registry[scene->GetUUID()] = scene;
        m_Active = scene->GetUUID();

        error.clear();
        const bool emptyFile = exists && std::filesystem::file_size(scenePath, error) == 0;
        if (!exists || (!error && emptyFile))
            scene->Save();

        ANV_LOG_INFO("Opened scene '%s'.", scenePath.string().c_str());
        return scene;
    }

    Ref<Scene> SceneManager::CreateScene(const std::filesystem::path& _path)
    {
        const std::filesystem::path scenePath = NormalizeScenePath(_path);
        if (scenePath.empty())
            return nullptr;

        std::error_code error;
        if (std::filesystem::exists(scenePath, error) &&
            std::filesystem::file_size(scenePath, error) > 0)
        {
            ANV_LOG_ERROR("Cannot create scene '%s' because it already exists.", scenePath.string().c_str());
            return nullptr;
        }

        return OpenScene(scenePath, true);
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

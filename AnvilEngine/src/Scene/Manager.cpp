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
		// First scene becomes active by default
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
		else {
			ANV_LOG_WARN("Scene path %s did not match with any file system mounts!");
			newPath = std::filesystem::path(_path);
		}

		Ref<Scene> scene = Ref<Scene>::Create(newPath);
		m_Registry.emplace(scene->m_UUID, scene);

		// If no active scene, set it
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
		ANV_ASSERT(m_Registry.find(_sceneUUID) != m_Registry.end(), 
			"Scene must be registered with the scene manager in order to be set as the active scene")
		m_Active = _sceneUUID;
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
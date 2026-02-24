#include "Manager.h"
#include "Scene.h"

namespace anv
{
	SceneManager::SceneManager()
	{
		ANV_LOG_INFO("Initializing Scene Manager")
	}

	SceneManager::~SceneManager()
	{
		for (auto& [uuid, scene] : m_Registry)
		{
			scene->Shutdown();
		}

		m_Registry.clear();
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
		SceneRegInfo info{
			.path = _path
		};
		Serializer ser(_path, Serializer::Mode::SER_MODE_TOML,
			Serializer::Direction::Read);
		ser.ObjectStrict("Scene", [&] {
			ser.FieldOr<std::string>("Name", info.name, "NewScene");
			ser.FieldOr<std::string>("UUID", info.uuid.uuid, "");
			ser.EnumFieldOr("Context", info.ctx, Scene::Context::CTX_2D,
				SceneContextToString, SceneContextFromString);
		});

		Ref<Scene> scene = Ref<Scene>::Create(info.name);
		if (info.uuid.uuid != "")
			scene->m_UUID = info.uuid;
		scene->m_Context = info.ctx;
		scene->m_Path = info.path;

		m_Registry.emplace(scene->m_UUID, scene);

		// If no active scene, set it
		if (m_Active.uuid.empty())
			m_Active = info.uuid;

		// load entities
		ser.ForEachTable("Entities", [&](const std::string& entUUID)
			{
				// We are now inside [Entities."<entUUID>"]
				std::string name;
				ser.FieldOr<std::string>("Name", name, "Entity");

				auto ent = scene->RegisterEntity(name, uuid::EntityUUID(entUUID));

				//ser.ObjectIf("Transform2d", [&] {
				     // TODO: Need to retrieve transform2d data then pass it in to AddCompnent 
				//	scene->AddComponent<Component::Transform2d>(ent);
				//});

				// Create entity using entUUID + name
			});

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
}
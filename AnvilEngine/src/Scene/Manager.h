#pragma once

#include "Scene.h"
#include "../Core/Uuid.h"
#include "../Core/Reference.h"
#include "../Util/Serialize/Serializer.h"

namespace anv
{

	class SceneManager
	{
	public:
		struct SceneRegInfo
		{
			std::string path;
			std::string name;
			uuid::AssetUUID uuid;
			Scene::Context ctx;
		};

		SceneManager();
		~SceneManager();

		Ref<Scene> Create(std::string _name);

		// Register is for "create a scene with externally supplied info"
		// e.g. from disk metadata, editor registry, etc.
		Ref<Scene> Register(std::string _path);

		Ref<Scene> Load(std::string _path);
		Ref<Scene> GetActive();

		void SetActive(uuid::AssetUUID _sceneUUID);
		void Unload(uuid::AssetUUID _sceneUUID);

	private:
		uuid::AssetUUID  m_Active;
		std::unordered_map<uuid::AssetUUID, Ref<Scene>> m_Registry;
	};
	
}

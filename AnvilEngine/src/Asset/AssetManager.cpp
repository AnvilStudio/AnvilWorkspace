#include "AssetManager.h"
#include "Util/FileSys/FileSystem.h"
#include <Core/App.h>
#include "AssetTypes/Texture.h"

namespace anv
{
	AssetManager::AssetManager()
		: m_Fs(App::GetInstance()->GetFS())
	{
		m_Fs.MountKey("AssetMeta", "Assets/com.anvstu.engine/AssetMeta/");
		resolve_assets();
	}

	AssetManager::~AssetManager()
	{
		// Ensure all assets are saved
		for (auto a : m_AssetReg)
		{
			a.second->Save();
		}
	}

	Ref<Asset> AssetManager::Get(const uuid::AssetUUID& _id) const
	{
		auto it = m_AssetReg.find(_id);
		if (it == m_AssetReg.end())
			return nullptr;
		return it->second;
	}

	void AssetManager::create(Deserialized& _dser)
	{
		if (_dser.type == "Shader")
		{
			auto shader = Shader::Create(_dser);
			shader->GenMetaFile();
			m_AssetReg.try_emplace(shader->GetAssetID(), shader);
		}

		if (_dser.type == "Texture")
		{
			ANV_LOG_DEBUG("Texture");
			Create<Texture>(_dser)->GenMetaFile();
		}
	}

	void AssetManager::resolve_assets()
	{
		auto assets = m_Fs.GetKeyVal("AssetMeta");

		m_Fs.ForEach(assets, [&](Ref<File> _file)
		{
				Serializer ser(_file, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Read);
				Deserialized dser;
				dser.ser = ser;

				ser.ObjectStrict("Asset", [&] {
					ser.FieldStrict("Name", dser.name);
					ser.FieldStrict("Resource", dser.resource);
					ser.FieldStrict("UUID", dser.uuid);
					ser.ObjectStrict("Spec", [&] {
					//	ser.FieldStrict("Cache", cache);
					//	ser.FieldStrict("ShaderName", shaderName);
					ser.FieldStrict("Type", dser.type);
					});
				});

				create(dser);
		});
		
	}
}
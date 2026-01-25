#include "AssetManager.h"
#include <Core/App.h>

namespace anv
{
	AssetManager::AssetManager()
	{
		auto& fs = App::GetInstance()->GetFS();
		fs.CreateKeyDir("AssetMeta", "Assets/com.anvstu.engine/AssetMeta");
	}

	AssetManager::~AssetManager()
	{
		// Ensure all assets are saved
		for (auto a : m_AssetReg)
		{
			a.second->Save();
		}
	}

	Ref<Asset> AssetManager::Create(std::string& _resource)
	{
		auto a = Ref<Asset>::Create(_resource);
		m_AssetReg.emplace(a->GetAssetID(), a);
		return a;
	}
}
#include "AssetManager.h"
#include <Core/App.h>

namespace anv
{
	AssetManager::AssetManager()
	{
		auto& fs = App::GetInstance()->GetFS();
		fs.CreateKeyDir("AssetMeta", "Assets/com.anvstu.engine/AssetMeta/");
	}

	AssetManager::~AssetManager()
	{
		// Ensure all assets are saved
		for (auto a : m_AssetReg)
		{
			a.second->Save();
		}
	}

	Ref<Asset> AssetManager::GetOrCreate(const std::string& _resource)
	{
		auto it = m_ByResource.find(_resource);
		if (it != m_ByResource.end())
		{
			auto it2 = m_AssetReg.find(it->second);
			if (it2 != m_AssetReg.end())
				return it2->second;
		}

		auto a = Ref<Asset>::Create(_resource);
		m_ByResource[_resource] = a->GetAssetID();
		m_AssetReg.emplace(a->GetAssetID(), a);
		return a;
	}
}
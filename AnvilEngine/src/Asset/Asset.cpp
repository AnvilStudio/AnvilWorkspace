#include "Asset.h"
#include "Core/App.h"

namespace anv {
	void Asset::GenAssetFile()
	{
		auto& fs = App::GetInstance()->GetFS();
		m_Meta = fs.AtKeyDir("AssetMeta") + m_Name + ".aamta";
		ANV_ASSERT(!m_Meta.empty(), "AssetMeta directory not registered");

		Serializer ser(m_Meta,
			Serializer::Mode::SER_MODE_TOML,
			Serializer::Direction::Write);

		ser.Object("Asset", [&]
			{
				ser.Field("Name", m_Name);
				ser.Field("Resource", m_ResPath);
				ser.Field("UUID", m_Uuid.uuid);
			});
	}

	void Asset::Save() 
	{
		Serializer ser(m_Meta,
			Serializer::Mode::SER_MODE_TOML,
			Serializer::Direction::Write);

		ser.Object("Asset", [&]
			{
				ser.Field("Name", m_Name);
				ser.Field("Resource", m_ResPath);
				ser.Field("UUID", m_Uuid.uuid);
			});
	}


	std::string Asset::RetrieveFileName(std::string& _path)
	{
		size_t pos = _path.find_last_of("/\\");

		if (pos != std::string::npos)
		{
			m_ResPath = _path.substr(0, pos);
			m_Name = _path.substr(pos + 1);
		}

		return m_Name;
	}
}
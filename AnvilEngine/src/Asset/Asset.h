#pragma once
#include "../Core/Reference.h"
#include "../Core/Uuid.h"

namespace anv{

	class Asset
		: public RefCounter
	{
	public:

		enum class Type
		{
			SHADER,
			MODEL,
			TEXTURE,
			IMAGE
		};

		Asset(std::string& _name)
			: m_Name(_name), m_Uuid(uuid::uuid_GenAssetID())
		{
			ANV_LOG_INFO("Creating Asset" + m_Name)
			RetrieveFileName(_name);
		}

		uuid::AssetUUID GetAssetID()
		{
			return m_Uuid;
		}

		void Save();
	
	protected:
		std::string RetrieveFileName(std::string& _path);
		void GenAssetFile();

	private:
		uuid::AssetUUID m_Uuid{};
		std::string m_ResPath;
		std::string m_Name;
		std::string m_Meta;
	};
}

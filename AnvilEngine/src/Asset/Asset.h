#pragma once
#include "../Core/Reference.h"
#include "../Core/Uuid.h"

namespace anv{

	class Asset
		: public RefCounter
	{
	public:

		Asset()
			: m_Uuid(uuid::uuid_GenAssetID())
		{

		}

		uuid::AssetUUID GetAssetID()
		{
			return m_Uuid;
		}
	
	private:
		// TODO: Impl serializer
		virtual void OnSerialize(/*Writer* _serializer*/) {};

	private:
		uuid::AssetUUID m_Uuid{};
	};
}

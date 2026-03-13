#pragma once

#include "../Asset.h"
#include "../AssetManager.h"

namespace anv
{
	class Texture
		: public Asset
	{
	public:
		~Texture();

		void Load();
		void Unload();

		int Width();
		int Height();
		int Channels();
		unsigned char* Data();

	protected:
		Texture(const std::filesystem::path&  _path);
		Texture(Deserialized& _dser);
		void OnSave(Serializer& _ser) override;

		friend class AssetManager;

	private:
		int m_Width;
		int m_Height;
		int m_Channels;
		unsigned char* m_Data;

	};
}

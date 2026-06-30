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
		static Ref<Texture> Create(std::filesystem::path& _path);
		Texture(const std::filesystem::path&  _path);
		Texture(Deserialized& _dser);
		void OnSave(Serializer& _ser) override;

		friend class AssetManager;
		friend class Ref <Texture>;

		int m_Width = 0;
		int m_Height = 0;
		int m_Channels = 0;
		unsigned char* m_Data = nullptr;

	};
}

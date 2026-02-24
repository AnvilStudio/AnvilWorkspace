#pragma once

#include "../Asset.h"

namespace anv
{
	class Texture
		: public Asset
	{
	public:
		
		Texture(std::string _path);
		Texture(Deserialized& _dser);
		~Texture();

		void Load();
		void Unload();

		int Width();
		int Height();
		int Channels();
		unsigned char* Data();

	protected:
		void OnSave(Serializer& _ser) override;

	private:
		int m_Width;
		int m_Height;
		int m_Channels;
		unsigned char* m_Data;
	};
}

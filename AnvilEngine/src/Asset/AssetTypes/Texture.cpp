#include "Texture.h"
#include "Util/Serialize/Serializer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace anv
{

	Texture::Texture(const std::filesystem::path& _path)
		: Asset(_path)
	{
		Load();
	}

	Texture::Texture(Deserialized& _dser)
		: Asset(_dser.resource)
	{
		Load();
	}

	Texture::~Texture()
	{
		if (m_Data)
			stbi_image_free(m_Data);
	}

	void Texture::Load()
	{
		stbi_set_flip_vertically_on_load(true);
		m_Data = stbi_load(m_ResourcePath.string().c_str(), 
			&m_Width, &m_Height, &m_Channels, 0);

		if (!m_Data)
		{
			const char* failure_reason = stbi_failure_reason();
			ANV_LOG_ERROR("Failed to load texture: %s\nReason: %s", m_ResourcePath.c_str(), failure_reason);
		}

		ANV_LOG_DEBUG("Loaded Text: " + m_Name)
	}
	
	void Texture::Unload()
	{
		if (m_Data)
			stbi_image_free(m_Data);
	}

	int Texture::Width()
	{
		return m_Width;
	}

	int Texture::Height()
	{
		return m_Height;
	}

	int Texture::Channels()
	{
		return m_Channels;
	}

	unsigned char* Texture::Data()
	{
		return m_Data;
	}

	void Texture::OnSave(Serializer& _ser)
	{
		_ser.Object("Spec", [&] {
			_ser.Field("Type", "Texture");
			_ser.Field("Width", m_Width);
			_ser.Field("Height", m_Height);
			_ser.Field("Channels", m_Channels);
		});
	}
}
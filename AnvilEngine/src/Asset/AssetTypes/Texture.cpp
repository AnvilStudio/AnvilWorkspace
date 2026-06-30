#include "Texture.h"
#include "Util/Serialize/Serializer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include <Core/App.h>

#include <Render/RenderAPI.h>
#include <Render/Platform/Vulkan/VulkanTexture2d.h>

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
	}

	Ref<Texture> Texture::Create(std::filesystem::path& _path)
	{
		auto ctx = App::GetInstance()->GetMainWindow()->GetContext();
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanTexture2D>::Create(ctx);
		}
	}

	Texture::~Texture()
	{
		Unload();
	}

	void Texture::Load()
	{
		Unload();

		stbi_set_flip_vertically_on_load(true);

		m_Data = stbi_load(
			m_ResourcePath.string().c_str(),
			&m_Width,
			&m_Height,
			&m_Channels,
			STBI_rgb_alpha
		);

		if (!m_Data)
		{
			ANV_LOG_ERROR(
				"Failed to load texture: %s\nReason: %s",
				m_ResourcePath.string().c_str(),
				stbi_failure_reason()
			);

			m_Width = 0;
			m_Height = 0;
			m_Channels = 0;
			return;
		}

		m_Channels = 4;

		ANV_LOG_DEBUG("Loaded Texture: " + m_Name);
	}

	void Texture::Unload()
	{
		if (m_Data)
		{
			stbi_image_free(m_Data);
			m_Data = nullptr;
		}

		m_Width = 0;
		m_Height = 0;
		m_Channels = 0;
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
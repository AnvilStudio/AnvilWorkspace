#include "Texture.h"
#include "Util/Serialize/Serializer.h"
#include "Render/RenderAPI.h"

#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
#include "Render/Platform/Metal/MtlTexture.h"
#endif

#if defined(PLATFORM_WIN64) || defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE_VK)
#include "Render/Platform/Vulkan/VulkanTexture.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace anv
{
    Texture::Texture(const std::filesystem::path& path)
        : Asset(path)
    {
        m_Type = "Texture";
        Load();
    }

    Texture::Texture(Deserialized& deserialized)
        : Asset(deserialized)
    {
        Load();
    }

    Texture::Texture(const std::string& internalName)
        : Asset(internalName)
    {
        m_Type = "Texture";
    }

    Texture::~Texture()
    {
        Unload();
    }

    Ref<Texture> Texture::Create(const std::filesystem::path& path)
    {
        switch (RenderAPI::GetAPI())
        {
        case GraphicsAPI::MTL:
#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
            return Ref<MetalTexture>::Create(path);
#else
            break;
#endif
        case GraphicsAPI::VK:
#if defined(PLATFORM_WIN64) || defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE_VK)
            return Ref<VulkanTexture>::Create(path);
#else
            break;
#endif
        default:
            break;
        }

        ANV_LOG_ERROR("Texture backend is unavailable for the active graphics API");
        return nullptr;
    }

    Ref<Texture> Texture::Create(Deserialized& deserialized)
    {
        switch (RenderAPI::GetAPI())
        {
        case GraphicsAPI::MTL:
#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
            return Ref<MetalTexture>::Create(deserialized).As<Texture>();
#else
            break;
#endif
        case GraphicsAPI::VK:
#if defined(PLATFORM_WIN64) || defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE_VK)
            return Ref<VulkanTexture>::Create(deserialized).As<Texture>();
#else
            break;
#endif
        default:
            break;
        }

        ANV_LOG_ERROR("Texture backend is unavailable for the active graphics API");
        return nullptr;
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
            STBI_rgb_alpha);

        if (!m_Data)
        {
            ANV_LOG_ERROR(
                "Failed to load texture: %s\nReason: %s",
                m_ResourcePath.string().c_str(),
                stbi_failure_reason());
            return;
        }

        m_Channels = 4;
        ANV_LOG_DEBUG("Loaded texture: " + m_Name)
    }

    void Texture::Unload()
    {
        if (m_Data)
        {
            stbi_image_free(m_Data);
            m_Data = nullptr;
        }
    }

    void Texture::OnSave(Serializer& serializer)
    {
        serializer.Object("Spec", [&]
        {
            serializer.Field("Type", "Texture");
            serializer.Field("Width", m_Width);
            serializer.Field("Height", m_Height);
            serializer.Field("Channels", m_Channels);
        });
    }
}

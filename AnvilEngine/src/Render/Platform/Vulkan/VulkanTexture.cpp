#include "VulkanTexture.h"

namespace anv
{
    VulkanTexture::VulkanTexture(const std::filesystem::path& path)
        : Texture(path)
    {
        Upload();
    }

    VulkanTexture::VulkanTexture(Deserialized& deserialized)
        : Texture(deserialized)
    {
        Upload();
    }

    VulkanTexture::~VulkanTexture()
    {
        Destroy();
    }

    bool VulkanTexture::IsGPUReady() const
    {
        return m_Image != nullptr;
    }

    void* VulkanTexture::GetNativeHandle() const
    {
        return m_Image;
    }

    void VulkanTexture::Upload()
    {
        // Vulkan GPU upload intentionally remains isolated here.
        // Metal is the active development backend; this class preserves
        // the engine texture API until Vulkan image allocation is restored.
        m_Image = nullptr;
    }

    void VulkanTexture::Destroy()
    {
        m_Image = nullptr;
    }
}

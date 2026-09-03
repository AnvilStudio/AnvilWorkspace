#pragma once

#include <Asset/AssetTypes/Texture.h>
#include <Render/Context.h>
#include <vulkan/vulkan.h>

namespace anv
{
    class VulkanTexture final : public Texture
    {
    public:
        explicit VulkanTexture(const std::filesystem::path& path);
        explicit VulkanTexture(Deserialized& deserialized);
        VulkanTexture(const unsigned char* rgbaPixels, int width, int height, const std::string& name);
        ~VulkanTexture() override;

        bool IsGPUReady() const override;
        void* GetNativeHandle() const override;

        VkImage GetImage() const { return m_Image; }
        VkImageView GetImageView() const { return m_ImageView; }
        VkSampler GetSampler() const { return m_Sampler; }
        VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
        VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

    private:
        void Upload();
        void Destroy();
        void create_image();
        void create_image_view();
        void create_sampler();
        void create_descriptor();

    private:
        _shared<Context> m_Context = nullptr;

        VkImage m_Image = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
    };
}

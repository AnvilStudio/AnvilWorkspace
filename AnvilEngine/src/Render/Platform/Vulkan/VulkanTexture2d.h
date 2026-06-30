#pragma once

#include <Core/Reference.h>
#include <Asset/AssetTypes/Texture.h>
#include <vulkan/vulkan.h>
#include "VulkanImage.h"

namespace anv
{
    class VulkanTexture2D
        : Texture
    {
    public:
        VulkanTexture2D(_shared<Context> ctx, std::filesystem::path& path);
        ~VulkanTexture2D();

        void Upload();
        void Destroy();

        // holy shit we gotta fix this...
        VkImageView GetImageView() const { return m_Image.As<VulkanImage2D>()->
            MakeImageView().As<VulkanImageView>()->Get(); }

        VkSampler GetSampler() const { return m_Sampler; }

    private:
        void create_sampler();

        void transition_image_layout(
            VkImage image,
            VkImageLayout oldLayout,
            VkImageLayout newLayout
        );

        void copy_buffer_to_image(
            VkBuffer buffer,
            VkImage image,
            uint32_t width,
            uint32_t height
        );
    private:
        _shared<Context> m_Context;
        Ref<Image2D> m_Image;
  
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}
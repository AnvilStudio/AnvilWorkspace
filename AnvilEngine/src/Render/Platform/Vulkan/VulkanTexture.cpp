#include "VulkanTexture.h"

#include "Core/App.h"
#include "Core/Window.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"

#include <cstdlib>
#include <cstring>

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

    VulkanTexture::VulkanTexture(const unsigned char* rgbaPixels, int width, int height, const std::string& name)
        : Texture(name)
    {
        if (!rgbaPixels || width <= 0 || height <= 0)
            return;

        m_Width = width;
        m_Height = height;
        m_Channels = 4;

        const size_t size = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        m_Data = static_cast<unsigned char*>(std::malloc(size));
        if (!m_Data)
        {
            ANV_LOG_ERROR("Failed to allocate internal Vulkan texture data");
            return;
        }

        std::memcpy(m_Data, rgbaPixels, size);
        Upload();
    }

    VulkanTexture::~VulkanTexture()
    {
        Destroy();
    }

    bool VulkanTexture::IsGPUReady() const
    {
        return m_Image != VK_NULL_HANDLE &&
               m_ImageView != VK_NULL_HANDLE &&
               m_Sampler != VK_NULL_HANDLE &&
               m_DescriptorSet != VK_NULL_HANDLE;
    }

    void* VulkanTexture::GetNativeHandle() const
    {
        return (void*)m_ImageView;
    }

    void VulkanTexture::Upload()
    {
        Destroy();

        if (!m_Data || m_Width <= 0 || m_Height <= 0)
            return;

        auto app = App::GetInstance();
        if (!app || !app->GetMainWindow())
        {
            ANV_LOG_ERROR("VulkanTexture could not acquire the application window");
            return;
        }

        m_Context = app->GetMainWindow()->GetContext();
        auto vkctx = m_Context ? m_Context->GetAs<VulkanContext>() : nullptr;
        if (!vkctx)
        {
            ANV_LOG_ERROR("VulkanTexture could not acquire a VulkanContext");
            m_Context = nullptr;
            return;
        }

        VkDevice device = vkctx->GetDevice();
        VkPhysicalDevice physicalDevice = vkctx->GetPhysicalDevice();
        const VkDeviceSize imageSize =
            static_cast<VkDeviceSize>(m_Width) *
            static_cast<VkDeviceSize>(m_Height) * 4;

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        vk_util::create_buffer(
            physicalDevice,
            device,
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingMemory);

        void* mapped = nullptr;
        ANV_VK_CHECK_RESULT(
            vkMapMemory(device, stagingMemory, 0, imageSize, 0, &mapped),
            "Failed to map Vulkan texture staging memory");
        std::memcpy(mapped, m_Data, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingMemory);

        create_image();

        vkctx->ImmediateSubmit([&](VkCommandBuffer cmd)
        {
            VkImageMemoryBarrier toTransfer{};
            toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toTransfer.image = m_Image;
            toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            toTransfer.subresourceRange.baseMipLevel = 0;
            toTransfer.subresourceRange.levelCount = 1;
            toTransfer.subresourceRange.baseArrayLayer = 0;
            toTransfer.subresourceRange.layerCount = 1;
            toTransfer.srcAccessMask = 0;
            toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &toTransfer);

            VkBufferImageCopy copy{};
            copy.bufferOffset = 0;
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel = 0;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageOffset = { 0, 0, 0 };
            copy.imageExtent = {
                static_cast<uint32_t>(m_Width),
                static_cast<uint32_t>(m_Height),
                1
            };

            vkCmdCopyBufferToImage(
                cmd,
                stagingBuffer,
                m_Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &copy);

            VkImageMemoryBarrier toShaderRead{};
            toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            toShaderRead.image = m_Image;
            toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            toShaderRead.subresourceRange.baseMipLevel = 0;
            toShaderRead.subresourceRange.levelCount = 1;
            toShaderRead.subresourceRange.baseArrayLayer = 0;
            toShaderRead.subresourceRange.layerCount = 1;
            toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &toShaderRead);
        });

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);

        create_image_view();
        create_sampler();
        create_descriptor();

        ANV_LOG_DEBUG("Uploaded Vulkan texture: " + m_Name)
    }

    void VulkanTexture::create_image()
    {
        auto vkctx = m_Context->GetAs<VulkanContext>();
        VkDevice device = vkctx->GetDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(m_Width);
        imageInfo.extent.height = static_cast<uint32_t>(m_Height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        ANV_VK_CHECK_RESULT(
            vkCreateImage(device, &imageInfo, nullptr, &m_Image),
            "Failed to create Vulkan texture image");

        VkMemoryRequirements requirements{};
        vkGetImageMemoryRequirements(device, m_Image, &requirements);

        VkMemoryAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = vk_util::vku_FindMemoryType(
            vkctx->GetPhysicalDevice(),
            requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        ANV_VK_CHECK_RESULT(
            vkAllocateMemory(device, &allocation, nullptr, &m_Memory),
            "Failed to allocate Vulkan texture memory");

        ANV_VK_CHECK_RESULT(
            vkBindImageMemory(device, m_Image, m_Memory, 0),
            "Failed to bind Vulkan texture memory");
    }

    void VulkanTexture::create_image_view()
    {
        VkImageViewCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = m_Image;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        ANV_VK_CHECK_RESULT(
            vkCreateImageView(
                m_Context->GetAs<VulkanContext>()->GetDevice(),
                &info,
                nullptr,
                &m_ImageView),
            "Failed to create Vulkan texture image view");
    }

    void VulkanTexture::create_sampler()
    {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.anisotropyEnable = VK_FALSE;
        info.maxAnisotropy = 1.0f;
        info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        info.unnormalizedCoordinates = VK_FALSE;
        info.compareEnable = VK_FALSE;
        info.compareOp = VK_COMPARE_OP_ALWAYS;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.mipLodBias = 0.0f;
        info.minLod = 0.0f;
        info.maxLod = 0.0f;

        ANV_VK_CHECK_RESULT(
            vkCreateSampler(
                m_Context->GetAs<VulkanContext>()->GetDevice(),
                &info,
                nullptr,
                &m_Sampler),
            "Failed to create Vulkan texture sampler");
    }

    void VulkanTexture::create_descriptor()
    {
        VkDevice device = m_Context->GetAs<VulkanContext>()->GetDevice();

        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        ANV_VK_CHECK_RESULT(
            vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout),
            "Failed to create Vulkan texture descriptor layout");

        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        ANV_VK_CHECK_RESULT(
            vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool),
            "Failed to create Vulkan texture descriptor pool");

        VkDescriptorSetAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocation.descriptorPool = m_DescriptorPool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &m_DescriptorSetLayout;

        ANV_VK_CHECK_RESULT(
            vkAllocateDescriptorSets(device, &allocation, &m_DescriptorSet),
            "Failed to allocate Vulkan texture descriptor set");

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = m_Sampler;
        imageInfo.imageView = m_ImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_DescriptorSet;
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    void VulkanTexture::Destroy()
    {
        if (!m_Context)
            return;

        auto vkctx = m_Context->GetAs<VulkanContext>();
        if (!vkctx)
        {
            m_Context = nullptr;
            return;
        }

        vkctx->IdleDevice();
        VkDevice device = vkctx->GetDevice();

        if (m_DescriptorPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
        if (m_DescriptorSetLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
        if (m_Sampler != VK_NULL_HANDLE)
            vkDestroySampler(device, m_Sampler, nullptr);
        if (m_ImageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, m_ImageView, nullptr);
        if (m_Image != VK_NULL_HANDLE)
            vkDestroyImage(device, m_Image, nullptr);
        if (m_Memory != VK_NULL_HANDLE)
            vkFreeMemory(device, m_Memory, nullptr);

        m_DescriptorSet = VK_NULL_HANDLE;
        m_DescriptorPool = VK_NULL_HANDLE;
        m_DescriptorSetLayout = VK_NULL_HANDLE;
        m_Sampler = VK_NULL_HANDLE;
        m_ImageView = VK_NULL_HANDLE;
        m_Image = VK_NULL_HANDLE;
        m_Memory = VK_NULL_HANDLE;
        m_Context = nullptr;
    }
}

#include "VulkanTexture2d.h"
#include "VulkanUtil.h"
#include "VulkanContext.h"
#include <Render/Buffer.h>
#include "VulkanBuffer.h"
namespace anv
{
	VulkanTexture2D::VulkanTexture2D(_shared<Context> ctx, std::filesystem::path& path)
		: Texture(path), m_Context(ctx)
	{
		Upload();
		create_sampler();
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		Destroy();
	}

	void VulkanTexture2D::Upload()
	{
		VkDeviceSize imageSize =
			m_Width *
			m_Height *
			4;

		BufferCreateInfo stagingInfo{};
		stagingInfo.Usage = BufferUsage::TransferSrc;
		stagingInfo.Size = imageSize;
		stagingInfo.InitialData = m_Data;

		Ref<Buffer> staging = Buffer::Create(m_Context, stagingInfo);
		auto vkStaging = staging.As<VulkanBuffer>();

		m_Image = Image2D::Create(
			m_Context,
			Image2D::Format::R8G8B8A8_SRGB,
			m_Width,
			m_Height
		);

		auto vkImage = m_Image.As<VulkanImage2D>();

		transition_image_layout(
			vkImage->Get(),
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		copy_buffer_to_image(
			vkStaging->GetBuffer(),
			vkImage->Get(),
			m_Width,
			m_Height
		);

		transition_image_layout(
			vkImage->Get(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	void VulkanTexture2D::Destroy()
	{
		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(
				m_Context->GetAs<VulkanContext>()->GetDevice(),
				m_Sampler,
				nullptr
			);

			m_Sampler = VK_NULL_HANDLE;
		}

		m_Image.Reset();
	}

	void VulkanTexture2D::create_sampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		ANV_VK_CHECK_RESULT(vkCreateSampler(m_Context->GetAs<VulkanContext>()->GetDevice(),
			&samplerInfo,
			nullptr,
			&m_Sampler
		), "Failed to create image sampler for texture!")
	}

	void VulkanTexture2D::transition_image_layout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();

		vkCtx->ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.oldLayout = oldLayout;
				barrier.newLayout = newLayout;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.image = image;

				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;

				VkPipelineStageFlags srcStage{};
				VkPipelineStageFlags dstStage{};

				if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
					newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
				{
					barrier.srcAccessMask = 0;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

					srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
					dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				}
				else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
					newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				{
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
					dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}
				else
				{
					ANV_LOG_ERROR("Unsupported texture layout transition!");
					return;
				}

				vkCmdPipelineBarrier(
					cmd,
					srcStage,
					dstStage,
					0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);
			});
	}

	void VulkanTexture2D::copy_buffer_to_image(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height)
	{
		auto vkCtx = m_Context->GetAs<VulkanContext>();

		vkCtx->ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkBufferImageCopy region{};
				region.bufferOffset = 0;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;

				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;

				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { width, height, 1 };

				vkCmdCopyBufferToImage(
					cmd,
					buffer,
					image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&region
				);
			});
	}
}
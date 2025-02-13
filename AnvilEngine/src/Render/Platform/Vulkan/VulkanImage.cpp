#include "VulkanImage.h"
#include "VulkanUtil.h"
namespace anv
{

	// VulkanImageView
	// =================================================================================================

	VulkanImageView::VulkanImageView(_shared<Context> _dv, VkImage _img, VkFormat _fmt)
		: m_VkContext(_dv->GetAs<VulkanContext>())
	{
		VkImageViewCreateInfo ivInfo{};
		ivInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ivInfo.image = _img;
		ivInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivInfo.format = _fmt;

		// R, G, B, A channel routing
		ivInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		ivInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		// describe pourpose, what part of the image to access
		// color target, no mipmap levels, one layer
		ivInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivInfo.subresourceRange.baseMipLevel = 0;
		ivInfo.subresourceRange.levelCount = 1;
		ivInfo.subresourceRange.baseArrayLayer = 0;
		ivInfo.subresourceRange.layerCount = 1;

		ANV_VK_CHECK_RESULT(vkCreateImageView(m_VkContext->GetDevice(), &ivInfo, nullptr, &m_ImgView),
			"Failed to create image view!");
	}

	VulkanImageView::~VulkanImageView()
	{
		vkDestroyImageView(m_VkContext->GetDevice(), m_ImgView, nullptr);
	}

	void VulkanImageView::OnDestroy()
	{
		ANV_PROFILE_SCOPE()

		vkDestroyImageView(m_VkContext->GetDevice(), m_ImgView, nullptr);
	}


	// VulkanImage2D
	// =================================================================================================

	VulkanImage2D::VulkanImage2D(_shared<Context> _ctx, Format _fmt)
		: Image2D(_ctx), m_Format(vk_util::vku_ToImageFormat(_fmt))
	{
		VkImageCreateInfo imgInfo{};
		
		imgInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType     = VK_IMAGE_TYPE_2D;
		imgInfo.extent.width  = m_Context->GetAs<VulkanContext>()->GetSwapchain()->GetExtent().width;
		imgInfo.extent.height = m_Context->GetAs<VulkanContext>()->GetSwapchain()->GetExtent().height;
		imgInfo.extent.depth  = 1;
		imgInfo.mipLevels     = 1;
		imgInfo.arrayLayers   = 1;
		imgInfo.format        = m_Format;
		imgInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imgInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

		ANV_VK_CHECK_RESULT(vkCreateImage(_ctx->GetAs<VulkanContext>()->GetDevice(), &imgInfo, nullptr, &m_Image),
			"Failed to create image!");
	}

	VulkanImage2D::VulkanImage2D(_shared<Context> _ctx, VkImage _img, Format _fmt)
		: Image2D(_ctx), m_Image(_img), m_Format(vk_util::vku_ToImageFormat(_fmt))
	{

	}

	VulkanImage2D::~VulkanImage2D()
	{
		ANV_PROFILE_SCOPE()

		//vkDestroyImage(m_Context->GetAs<VulkanContext>()->GetDevice(), m_Image, nullptr);
	}

	Ref<ImageView> VulkanImage2D::MakeImageView()
	{
		return Ref<VulkanImageView>::Create(m_Context, m_Image, m_Format);
	}
}
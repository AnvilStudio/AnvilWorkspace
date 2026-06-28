#include "VulkanImage.h"
#include "VulkanUtil.h"
namespace anv
{

	// VulkanImageView
	// =================================================================================================

	VulkanImageView::VulkanImageView(_shared<Context> _dv, VkFormat _fmt, VkImage _img)
		: m_VkContext(_dv->GetAs<VulkanContext>())
	{
		ANV_ASSERT(_img, "Image was NULL")
		VkImageViewCreateInfo ivInfo{};
		ivInfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
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
		ivInfo.subresourceRange.aspectMask       = VK_IMAGE_ASPECT_COLOR_BIT;
		ivInfo.subresourceRange.baseMipLevel    = 0;
		ivInfo.subresourceRange.levelCount        = 1;
		ivInfo.subresourceRange.baseArrayLayer = 0;
		ivInfo.subresourceRange.layerCount        = 1;

		ANV_VK_CHECK_RESULT(vkCreateImageView(m_VkContext->GetDevice(), &ivInfo, nullptr, &m_ImgView),
			"Failed to create image view!");
	}

	VulkanImageView::~VulkanImageView()
	{
		if (m_ImgView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_VkContext->GetDevice(), m_ImgView, nullptr);
			m_ImgView = VK_NULL_HANDLE;
		}
	}

	void VulkanImageView::OnDestroy()
	{
		ANV_PROFILE_SCOPE()

		if (m_ImgView != VK_NULL_HANDLE) {
			vkDestroyImageView(m_VkContext->GetDevice(), m_ImgView, nullptr);
			m_ImgView = VK_NULL_HANDLE;
		}
	}


	// VulkanImage2D
	// =================================================================================================

	VulkanImage2D::VulkanImage2D(_shared<Context> _ctx, Format _fmt, uint32_t _width, uint32_t _height)
		: Image2D(_ctx, _width, _height), m_Format(vk_util::vku_ToImageFormat(_fmt))
	{
		create_image();
	}

	VulkanImage2D::VulkanImage2D(_shared<Context> _ctx, VkImage _img, Format _fmt, uint32_t _width, uint32_t _height)
		: Image2D(_ctx, _width, _height), m_Image(_img), m_Format(vk_util::vku_ToImageFormat(_fmt))
	{

	}

	VulkanImage2D::~VulkanImage2D()
	{
		vkDestroyImage(m_Context->GetAs<VulkanContext>()->GetDevice(), m_Image, nullptr);
		vkFreeMemory(m_Context->GetAs<VulkanContext>()->GetDevice(), m_Memory, nullptr);
	}

	Ref<ImageView> VulkanImage2D::MakeImageView()
	{
		return Ref<VulkanImageView>::Create(m_Context, m_Format, m_Image);
	}

	void VulkanImage2D::create_image()
	{
		VkImageCreateInfo imgInfo{};

		imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imgInfo.imageType = VK_IMAGE_TYPE_2D;
		imgInfo.extent.width = m_Width;
		imgInfo.extent.height = m_Height;
		imgInfo.extent.depth = 1;
		imgInfo.mipLevels = 1;
		imgInfo.arrayLayers = 1;
		imgInfo.format = m_Format;
		imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imgInfo.usage =
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		auto vkctx = m_Context->GetAs<VulkanContext>();
		VkDevice device = vkctx->GetDevice();

		ANV_VK_CHECK_RESULT(
			vkCreateImage(device, &imgInfo, nullptr, &m_Image),
			"Failed to create image!"
		);

		VkMemoryRequirements memReq{};
		vkGetImageMemoryRequirements(device, m_Image, &memReq);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReq.size;
		allocInfo.memoryTypeIndex = vk_util::vku_FindMemoryType(
			vkctx->GetPhysicalDevice(),
			memReq.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		ANV_VK_CHECK_RESULT(
			vkAllocateMemory(device, &allocInfo, nullptr, &m_Memory),
			"Failed to allocate image memory!"
		);

		vkBindImageMemory(device, m_Image, m_Memory, 0);
	}
}
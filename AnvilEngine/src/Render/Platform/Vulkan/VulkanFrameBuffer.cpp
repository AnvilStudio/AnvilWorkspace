#include "VulkanFrameBuffer.h"
#include "VulkanImage.h"

namespace anv
{
	VulkanFrameBuffer::VulkanFrameBuffer(_shared<Context> _ctx, Ref<ImageView> _imgv, Ref<RenderPass> _rp)
		: Framebuffer(_ctx, _imgv, _rp)
	{
		ANV_LOG_DEBUG("Creating VkFrameBuffer")
		create_frame_buffer();
	}

	VulkanFrameBuffer::~VulkanFrameBuffer()
	{
		ANV_PROFILE_SCOPE()

		if (m_FrameBuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(m_Context->GetAs<VulkanContext>()->GetDevice(), m_FrameBuffer, nullptr);
			m_FrameBuffer = VK_NULL_HANDLE;
		}
	}

	void VulkanFrameBuffer::create_frame_buffer()
	{

		VkFramebufferCreateInfo fb_info{};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass      = m_RenderPass.As<VulkanRenderPass>()->GetRaw();
		fb_info.attachmentCount = 1;
		VkImageView attachments[] = { m_ImageView.As<VulkanImageView>()->GetRaw() };
		fb_info.pAttachments = attachments;
		fb_info.width = m_Context->GetSwapchain()->GetExtent().width;
		fb_info.height = m_Context->GetSwapchain()->GetExtent().height;
		fb_info.layers = 1;

		ANV_VK_CHECK_RESULT(vkCreateFramebuffer(m_Context->GetAs<VulkanContext>()->GetDevice(), &fb_info, nullptr, &m_FrameBuffer),
			"Failed to create Vk framebuffer!")
	}

}
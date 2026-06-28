#include "VulkanSwapChainTarget.h"
#include "VulkanSwapChain.h"

namespace anv
{
	uint32_t VulkanSwapchainRenderTarget::AcquireNextImage(
		VkSemaphore imageAvailable,
		bool& recreate)
	{
		m_ImageIndex = m_Context->GetSwapchain()
			.As<VulkanSwapchain>()
			->AcquireNextImage(imageAvailable, recreate, VK_NULL_HANDLE);

		return m_ImageIndex;
	}

	void VulkanSwapchainRenderTarget::Present(VkQueue presentQueue, VkSemaphore renderFinished, bool& recreate)
	{
		m_Context->GetSwapchain()
			.As<VulkanSwapchain>()->Present(presentQueue, m_ImageIndex, renderFinished, recreate);
	}

	void VulkanSwapchainRenderTarget::Resize(uint32_t _width, uint32_t _height)
	{
		m_Width = _width;
		m_Height = _height;

		create_framebuffers();
	}

	void VulkanSwapchainRenderTarget::Begin(Ref<CommandBuffer> _cmd)
	{
		m_Renderpass->Begin(_cmd, m_Framebuffers[m_ImageIndex], m_Width, m_Height);
	}

	void VulkanSwapchainRenderTarget::End(Ref<CommandBuffer> _cmd)
	{
		m_Renderpass->End(_cmd);
	}

	void VulkanSwapchainRenderTarget::create_framebuffers()
	{
		auto views = m_Context->GetSwapchain()->GetImageViews();

		m_Framebuffers.resize(views.size());

		for (size_t i = 0; i < views.size(); i++)
		{
			m_Framebuffers[i] = Framebuffer::Create(
				m_Context,
				views[i],
				m_Renderpass,
				m_Width,
				m_Height
			);
		}
	}

	void VulkanSwapchainRenderTarget::create_renderpass()
	{
		RenderPassCreateInfo rpinfo{};
		rpinfo.d_name = "SwapchainTargetPass";

		RenderPassAttachment colatt;
		colatt.imageFormat = vk_util::vku_ToEngineImgFormat(m_Context->GetSwapchain().As<VulkanSwapchain>()->GetFormat());
		colatt.type = RenderPassAttachment::Type::ATT_TY_COLOR;
		colatt.loadOp = RenderPassAttachment::LoadOp::LOAD_OP_CLEAR;
		colatt.storeOp = RenderPassAttachment::StoreOp::STORE_OP_STORE;
		colatt.beginLayout = RenderPassAttachment::ImgLayout::IMG_LAYOUT_UNDEF;
		colatt.endLayout = RenderPassAttachment::ImgLayout::IMG_LAYOUT_PRES;

		rpinfo.attachments.push_back(colatt);

		RenderPassCreateInfo::SubpassInfo sp{};
		sp.colorAttachments = { 0 };
		sp.depthStencilAttachment = -1;

		rpinfo.subpasses.push_back(sp);

		m_Renderpass = RenderPass::Create(rpinfo, m_Context);
		m_Renderpass->Build();
	}

}
#pragma once
#include "Render/RenderPass.h"
#include "VulkanContext.h"
#include "Render/Framebuffer.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace anv
{
	class VulkanRenderPass
		: public RenderPass
	{
	public:
		VulkanRenderPass(RenderPassCreateInfo _rpinfo, _shared<Context> _ctx);
		~VulkanRenderPass();

		void Begin() override;
		void End()	 override;
		void Build() override;

		VkRenderPass GetRaw();

		_vec<VkAttachmentDescription> GetAttachments();
		void SetFramebuffers(_vec<Ref<Framebuffer>> fbs) { m_Framebuffers = fbs; }


	private:
		void init_render_pass(RenderPassCreateInfo _rpinfo);

	private:
		VkRenderPass   m_RenderPass;
		VulkanContext* m_VkContext;
		_vec<std::pair<VkAttachmentDescription, int>> 
					   m_Descriptions;
		_vec<VkSubpassDescription> 
			           m_Subpasses;
		_vec<Ref<Framebuffer>>
			           m_Framebuffers = {};

	};
}

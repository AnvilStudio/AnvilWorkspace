#pragma once
#include "Render/RenderPass.h"
#include "VulkanContext.h"
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

		virtual RenderPassSignature& GetSignature() override;

		void Begin(Ref<CommandBuffer> _cmd, Ref<Framebuffer> _fb, uint32_t _width, uint32_t _height) override;
		void End(Ref<CommandBuffer> cmd)	 override;
		void Build() override;

		VkRenderPass Get();

		_vec<VkAttachmentDescription> GetAttachments();

	private:
		void init_render_pass(RenderPassCreateInfo _rpinfo);

	private:
		VkRenderPass   m_RenderPass;
		VulkanContext* m_VkContext; // just saw this again, will fix
		_vec<std::pair<VkAttachmentDescription, int>> 
					   m_Descriptions;
		_vec<VkSubpassDescription> 
			           m_Subpasses;
	};
}

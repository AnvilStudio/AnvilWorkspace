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

		void Begin() override {};
		void End()	 override {};
		void Build() override;

		VkRenderPass GetRaw();

	private:
		void init_render_pass(RenderPassCreateInfo _rpinfo);
		void parse_attachment(RenderPassCreateInfo::Attachment* _att, VkAttachmentDescription* _desc);
		void parse_layouts(RenderPassCreateInfo::Attachment* _att, VkAttachmentDescription* _desc);
	private:
		VkRenderPass   m_RenderPass;
		VulkanContext* m_VkContext;
		_vec<std::pair<VkAttachmentDescription, int>> 
					   m_Descriptions;
		_vec<VkSubpassDescription> 
			           m_Subpasses;
	};
}

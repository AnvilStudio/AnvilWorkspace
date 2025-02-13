#include "VulkanRenderPass.h"

anv::VulkanRenderPass::VulkanRenderPass(RenderPassCreateInfo _rpinfo, _shared<Context> _ctx)
	: RenderPass(_rpinfo.d_name), m_VkContext(_ctx->GetAs<VulkanContext>())
{
	m_VkContext = _ctx->GetAs<VulkanContext>();
	init_render_pass(_rpinfo);
}

anv::VulkanRenderPass::~VulkanRenderPass()
{
	ANV_PROFILE_SCOPE()

	vkDestroyRenderPass(m_VkContext->GetDevice(), m_RenderPass, nullptr);
}

void anv::VulkanRenderPass::Build()
{
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	_vec<VkAttachmentReference> color{};
	_vec<VkAttachmentReference> depth{};

	// Create color attachment refs
	for (auto& att : m_Descriptions)
	{
		VkAttachmentReference attRef = {};
		attRef.attachment = att.second;
		attRef.layout = att.first.finalLayout;
		color.push_back(attRef);
	}

	// Create depth attachment refs
	_vec<VkAttachmentDescription> result;
	result.reserve(m_Descriptions.size()); // Reserve space to avoid multiple reallocations
	for (const auto& pair : m_Descriptions) {
		result.push_back(pair.first);
	}

	subpass.colorAttachmentCount = static_cast<uint32_t>(color.size());
	subpass.pColorAttachments = color.data();
	// TODO: same for depth

	VkRenderPassCreateInfo rpinfo{};
	rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpinfo.attachmentCount = static_cast<uint32_t>(color.size());
	rpinfo.pAttachments = result.data();
	rpinfo.subpassCount =1;
	rpinfo.pSubpasses = &subpass;

	ANV_LOG_INFO("Creating Vk render pass \"%s\"", m_DName.c_str())
	ANV_VK_CHECK_RESULT(vkCreateRenderPass(m_VkContext->GetDevice(), &rpinfo, nullptr, &m_RenderPass), 
		"Failed to create Vk render pass")
}

VkRenderPass anv::VulkanRenderPass::GetRaw()
{
	return m_RenderPass;
}

anv::_vec<VkAttachmentDescription> anv::VulkanRenderPass::GetAttachments()
{
	_vec<VkAttachmentDescription> descr;
	for (auto desc : m_Descriptions)
	{
		descr.push_back(desc.first);
	}

	return descr;
}

void anv::VulkanRenderPass::init_render_pass(RenderPassCreateInfo _rpinfo)
{
	// create vk attachments with info provided
	int d_att_index = 0; // debug attachment index
	for (auto& attachment : _rpinfo.attachments)
	{
		attachment.d_index = d_att_index;

		VkAttachmentDescription desc = {};
		desc.format = m_VkContext->GetSwapchain()->GetFormat();
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		vk_util::vku_ToVulkanAttachmentDescription(&attachment, &desc);
		vk_util::vku_ToRenderPassLayout(&attachment, &desc);

		m_Descriptions.push_back(std::make_pair(desc, d_att_index));
		d_att_index++;
	}
}


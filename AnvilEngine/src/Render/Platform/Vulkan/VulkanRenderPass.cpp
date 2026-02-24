#include "VulkanRenderPass.h"
#include "VulkanCommandBuffer.h"
#include <Render/RenderFrameCtx.h>
#include "VulkanFrameBuffer.h"

anv::VulkanRenderPass::VulkanRenderPass(RenderPassCreateInfo _rpinfo, _shared<Context> _ctx)
	: RenderPass(_rpinfo), m_VkContext(_ctx->GetAs<VulkanContext>())
{
	m_VkContext = _ctx->GetAs<VulkanContext>();
	init_render_pass(_rpinfo);
}

anv::VulkanRenderPass::~VulkanRenderPass()
{
	ANV_PROFILE_SCOPE()
	ANV_LOG_INFO("Destroying Vulkan Render Pass \"%s\"", m_DName.c_str())
	if (!m_VkContext || !m_VkContext->GetDevice())
	{
		ANV_LOG_ERROR("Vulkan context or device is null, cannot destroy render pass \"%s\"!", m_DName.c_str());
		return;
	}
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
		attRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

	ANV_LOG_INFO("Creating Vk render pass " + m_DName)
	ANV_VK_CHECK_RESULT(vkCreateRenderPass(m_VkContext->GetDevice(), &rpinfo, nullptr, &m_RenderPass), 
		"Failed to create Vk render pass")
}

VkRenderPass anv::VulkanRenderPass::Get()
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

void anv::VulkanRenderPass::Begin()
{
	ANV_ASSERT(m_RenderQueue, "VulkanRenderPass::m_RenderQueue is null");

	m_RenderQueue->WriteToBack([&](Ref<CommandBuffer> cmd, const RenderFrameContext& frame) {

		ANV_ASSERT(frame.imageIndex < m_Framebuffers.size(), "imageIndex out of range");

		auto vkCmd = cmd.As<VulkanCommandBuffer>();
		ANV_ASSERT(vkCmd, "CommandBuffer cast failed!");

		auto vkFb = m_Framebuffers[frame.imageIndex].As<VulkanFrameBuffer>();
		ANV_ASSERT(vkFb, "Framebuffer cast failed!");

		VkRenderPassBeginInfo rp{};
		rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp.renderPass = m_RenderPass;
		rp.framebuffer = vkFb->Get();
		auto ext = m_VkContext->GetSwapchain()->GetExtent();
		rp.renderArea.offset = { 0, 0 };
		rp.renderArea.extent = { ext.width, ext.height };

		VkClearValue clearValues[1]{};
		clearValues[0].color = { {0.f, 0.f, 0.f, 1.f} };
		rp.clearValueCount = 1;
		rp.pClearValues = clearValues;

		vkCmdBeginRenderPass(vkCmd->Get(), &rp, VK_SUBPASS_CONTENTS_INLINE);
	});
}


void anv::VulkanRenderPass::End()
{
	m_RenderQueue->WriteToBack([](Ref<CommandBuffer> cmd, const RenderFrameContext&) {
		auto vkCmd = cmd.As<VulkanCommandBuffer>();
		vkCmdEndRenderPass(vkCmd->Get());
	});
}




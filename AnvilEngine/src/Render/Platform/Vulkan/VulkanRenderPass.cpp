#include "VulkanRenderPass.h"

anv::VulkanRenderPass::VulkanRenderPass(RenderPassCreateInfo _rpinfo, _shared<Context> _ctx)
{
	m_VkContext = _ctx->GetNativeContextAs<VulkanContext>();

	init_render_pass(_rpinfo);
}

anv::VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyRenderPass(m_VkContext->GetDevice(), m_RenderPass, nullptr);
}

void anv::VulkanRenderPass::Build()
{
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	_vec<VkAttachmentReference> color{};
	_vec<VkAttachmentReference> depth{};

	for (auto& att : m_Descriptions)
	{
		VkAttachmentReference attRef = {};
		attRef.attachment = att.second;
		attRef.layout = att.first.finalLayout;
		color.push_back(attRef);
	}

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

	ANV_VK_CHECK_RESULT(vkCreateRenderPass(m_VkContext->GetDevice(), &rpinfo, nullptr, &m_RenderPass), 
		"Failed to create Vk render pass")
}

VkRenderPass anv::VulkanRenderPass::GetRaw()
{
	return m_RenderPass;
}

void anv::VulkanRenderPass::init_render_pass(RenderPassCreateInfo _rpinfo)
{
	// create vk attachments with info provided
	int d_att_index = 0; // debug attachment index
	for (auto& attachment : _rpinfo.attachments)
	{
		attachment.d_index = d_att_index;

		VkAttachmentDescription desc = {};
		desc.format = m_VkContext->GetSwapchain().GetFormat();
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		parse_attachment(&attachment, &desc);
		parse_layouts(&attachment, &desc);

		m_Descriptions.push_back(std::make_pair(desc, d_att_index));
		d_att_index++;
	}
}

void anv::VulkanRenderPass::parse_attachment(RenderPassCreateInfo::Attachment* _att, VkAttachmentDescription* _desc)
{
	switch (_att->type)
	{
	// Color attachment
	case RenderPassCreateInfo::Attachment::AttType::ATT_TY_COLOR:
		// load ops
		switch (_att->loadOp)
		{
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_CLEAR:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_LOAD:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_UNDEF:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_MAX_ENUM:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ANV_LOG_WARN("Vk Render Pass attachment %i has an unusable load op (MAX_ENUM)\nSetting to clear op", _att->d_index)
				break;
		default:
			ANV_LOG_ERROR("Vk Render Pass attachment %i has an unknown load op", _att->d_index)
				break;
		}

		// store ops
		switch (_att->storeOp)
		{
		case RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_STORE:
			_desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			break;
		case RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_UNDEF:
			_desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			break;
		case RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_MAX_ENUM:
			_desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ANV_LOG_WARN("Vk Render Pass attachment %i has an unusable store op (MAX_ENUM)\nSetting to undef op", _att->d_index)
				break;
		default:
			ANV_LOG_ERROR("Vk Render Pass attachment %i has an unknown store op", _att->d_index)
				break;
		}
		break;

	// Depth Attachment
	case RenderPassCreateInfo::Attachment::AttType::ATT_TY_DEPTH:
		// load ops
		switch (_att->loadOp)
		{
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_CLEAR:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			break;
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_LOAD:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
			break;
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_UNDEF:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			break;
		case RenderPassCreateInfo::Attachment::LoadOp::LOAD_OP_MAX_ENUM:
			_desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ANV_LOG_WARN("Vk Render Pass attachment %i has an unusable load op (MAX_ENUM)\nSetting to clear op", _att->d_index)
				break;
		default:
			ANV_LOG_ERROR("Vk Render Pass attachment %i has an unknown load op", _att->d_index)
				break;
		}

		// store ops 
		switch (_att->storeOp)
		{
		case RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_STORE:
			_desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			break;
		case RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_UNDEF:
			_desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			break;
		case RenderPassCreateInfo::Attachment::StoreOp::STORE_OP_MAX_ENUM:
			_desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			ANV_LOG_WARN("Vk Render Pass attachment %i has an unusable store op (MAX_ENUM)\nSetting to undef op", _att->d_index)
			break;
		default:
			ANV_LOG_ERROR("Vk Render Pass attachment %i has an unknown store op", _att->d_index)
			break;
		}
		break;
	}
}

void anv::VulkanRenderPass::parse_layouts(RenderPassCreateInfo::Attachment* _att, VkAttachmentDescription* _desc)
{
	switch (_att->beginLayout)
	{
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_COLOR_ATT:
		_desc->initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_PRES:
		_desc->initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_MEMCPY_DST:
		_desc->initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_UNDEF:
		_desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_MAX_ENUM:
		ANV_LOG_WARN("Vk render pass %i has an unusable begining layout (MAX_ENUM)\nSetting to undef", _att->d_index)
		_desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		break;
	default:
		ANV_LOG_ERROR("Vk render pass %i has an unknown begining layout", _att->d_index);
		break;
	}

	switch (_att->endLayout)
	{
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_COLOR_ATT:
		_desc->finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_PRES:
		_desc->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_MEMCPY_DST:
		_desc->finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_UNDEF:
		_desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		break;
	case RenderPassCreateInfo::Attachment::ImgLayout::IMG_LAYOUT_MAX_ENUM:
		ANV_LOG_WARN("Vk render pass %i has an unusable final layout (MAX_ENUM)\nSetting to undef", _att->d_index)
			_desc->finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		break;
	default:
		ANV_LOG_ERROR("Vk render pass %i has an unknown final layout", _att->d_index);
		break;
	}
}


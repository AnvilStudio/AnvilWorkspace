#include "RenderPass.h"
#include "../Core/Reference.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "RenderAPI.h"

namespace anv
{
    Ref<RenderPass> RenderPass::Create(RenderPassCreateInfo& _createinfo, _shared<Context> _ctx)
    {
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanRenderPass>::Create(_createinfo, _ctx);

		default:
			ANV_LOG_ERROR("No matching API for render pass creation!");
			break;
		}
    }

	RenderPass::RenderPass(std::string _dname)
		: m_DName(_dname)
	{
	}
}
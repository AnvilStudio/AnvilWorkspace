#include "RenderPass.h"
#include "Platform/Vulkan/VulkanRenderPass.h"
#include "RenderAPI.h"

namespace anv
{
    _shared<RenderPass> RenderPass::Create(RenderPassCreateInfo& _createinfo, _shared<Context> _ctx)
    {
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return std::make_shared<VulkanRenderPass>(_createinfo, _ctx);

		default:
			ANV_LOG_ERROR("No matching API for render pass creation!");
			break;
		}
    }
}
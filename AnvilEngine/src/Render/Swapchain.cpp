#include "Swapchain.h"
#include "RenderAPI.h"
#include <Render/Platform/Vulkan/VulkanSwapchain.h>

namespace anv 
{
	_unique<Swapchain> Swapchain::Create(Context* _ctx)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return std::make_unique<VulkanSwapchain>(_ctx);
		default:
			//ANV_LOG_ERROR("Graphics API Swapchain not supported! using Vulkan swapchain")
			return std::make_unique<VulkanSwapchain>(_ctx);
			break;
		}
	}
}

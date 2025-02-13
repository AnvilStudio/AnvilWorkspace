#include "Swapchain.h"
#include "RenderAPI.h"
#include "Renderer.h"
#include <Render/Platform/Vulkan/VulkanSwapchain.h>

namespace anv 
{
	Ref<Swapchain> Swapchain::Create(_shared<Context> _ctx)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanSwapchain>::Create(_ctx);
		default:
			ANV_LOG_ERROR("Graphics API Swapchain not supported! using Vulkan swapchain")
			return Ref<VulkanSwapchain>::Create(_ctx);
			break;
		}
	}

	Swapchain::Swapchain(_shared<Context> _ctx)
		: m_Context(_ctx)
	{
	}
}

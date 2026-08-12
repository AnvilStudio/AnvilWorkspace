#include "Swapchain.h"
#include "RenderAPI.h"
#include "Renderer.h"

#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
#include <Render/Platform/Vulkan/VulkanSwapchain.h>
#elif defined(PLATFORM_APPLE)
// #include <Render/Platform/Metal/MtlSwapchain.h>
#endif

namespace anv 
{
	Ref<Swapchain> Swapchain::Create(_shared<Context> _ctx)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
			return Ref<VulkanSwapchain>::Create(_ctx);
#else
			ANV_LOG_FATAL("Machine does not support Vk Swapchain")
			return nullptr;
#endif
			break;
		case GraphicsAPI::MTL:
#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
			// TODO: IMPL //
			return nullptr;
#else
			ANV_LOG_FATAL("Machine does not support Metal Swapchain")
			return nullptr;
#endif
			break;
		default:
			ANV_LOG_ERROR("Could not properly detect graphics API!")
			return nullptr;
			break;
		}
	}

	Swapchain::Swapchain(_shared<Context> _ctx)
		: m_Context(_ctx)
	{
	}
}

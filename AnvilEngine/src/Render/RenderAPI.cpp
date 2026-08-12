#include "RenderAPI.h"
#include "Context.h"
#include "Framebuffer.h"
#include "Renderer.h"
#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
#include "Platform/Vulkan/VulkanRenderAPI.h"
#endif
#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
#include "Platform/Metal/MtlRenderAPI.h"
#endif

namespace anv
{

	_shared<RenderAPI> RenderAPI::Create(Render2DCreateInfo _info)
	{
		switch (s_API)
		{
		case GraphicsAPI::VK:
			#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
			return std::make_shared<VulkanRenderAPI>(_info);
			#else
			ANV_LOG_FATAL("Failed to create RenderAPI with Vulkan!")
			return nullptr;
			#endif
			break;

		case GraphicsAPI::MTL:
			#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
			return std::make_shared<MetalRenderAPI>(_info);
			#else
			ANV_LOG_FATAL("Failed to create RenderAPI with Metal!")
			return nullptr;
			#endif
			break;
			
		default:
			return nullptr;
			break;
		}
	}

}

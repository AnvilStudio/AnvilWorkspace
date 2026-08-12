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
			// Construct the base shared_ptr directly instead of going through
			// std::make_shared's __shared_ptr_emplace control block. This keeps
			// backend ownership identical while avoiding a libc++ control-block
			// instantiation failure seen in the macOS Vulkan build.
			return _shared<RenderAPI>(new VulkanRenderAPI(_info));
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

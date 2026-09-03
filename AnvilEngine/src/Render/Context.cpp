#include "Context.h"
#include "Core/Window.h"
#include "Renderer.h"

#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
 #include "Render/Platform/Vulkan/VulkanContext.h"
#endif
#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
 #include "Render/Platform/Metal/MtlContext.h"
#endif
#include "RenderAPI.h"
#include <GLFW/glfw3.h>

namespace anv {
    _shared<Context> Context::Create(Window* _win)
    {
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
				return std::make_shared<VulkanContext>(_win);
			#else
				ANV_LOG_FATAL("Vulkan was set as the Graphics API, but this machine is not supported!")
				return nullptr;
			#endif
			break;

		case GraphicsAPI::MTL:
			#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
				return std::make_shared<MetalContext>(_win);
			#else
				ANV_LOG_FATAL("Metal was set as the Graphics API, but this machine is not supported!")
				return nullptr;
			#endif
			break;

		default:
			ANV_LOG_FATAL("No Graphics API set or API is unknown!")
			return nullptr;
			break;
		}
    }

	Context::Context(Window* _win)
		: m_WinHandle(_win->GetNativeWindow())
	{

	}

	_shared<RenderAPI> Context::InitAPI(Render2DCreateInfo _info)
	{
		m_API = RenderAPI::Create(_info);
		return m_API;
	}
}
#include "Context.h"
#include "Core/Window.h"
#include "Render/Platform/Vulkan/VulkanContext.h"

#include <GLFW/glfw3.h>

namespace anv {
    _shared<Context> Context::Create(Window* _win)
    {
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return std::make_shared<VulkanContext>(_win);
			break;
		default:
			ANV_LOG_ERROR("Graphics API not supported! Using Vulkan")
			break;
		}
    }

	Context::Context(Window* _win)
		: m_WinHandle(_win->GetNativeWindow())
	{

	}

	_shared<RenderAPI> Context::InitAPI(RenderAPICreateInfo _info)
	{
		m_API = RenderAPI::Create(_info);
		return m_API;
	}
}
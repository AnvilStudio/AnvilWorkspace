#include "Context.h"
#include "Render/Platform/Vulkan/VulkanContext.h"
#include "Core/Window.h"

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

	Context::Context()
	{
	}

	_shared<RenderAPI> Context::InitAPI(RenderAPICreateInfo _info)
	{
		m_API = RenderAPI::Create(_info);
		return m_API;
	}
}
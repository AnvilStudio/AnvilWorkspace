#include "RenderAPI.h"
#include "Context.h"
#include "Framebuffer.h"
#include "Renderer.h"
#include "Platform/Vulkan/VulkanRenderAPI.h"

namespace anv
{

	_shared<RenderAPI> RenderAPI::Create(Render2DCreateInfo _info)
	{
		switch (s_API)
		{
		case anv::GraphicsAPI::VK:
			return std::make_shared<VulkanRenderAPI>(_info);
		case anv::GraphicsAPI::OGL:
			break;
		case anv::GraphicsAPI::DX:
			break;
		case anv::GraphicsAPI::MTL:
			break;
		default:
			break;
		}
	}

}

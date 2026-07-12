#include "RenderTarget.h"
#ifdef PLATFORM_WIN64
#include "Platform/Vulkan/VulkanRenderTarget.h"
#elif defined(PLATFORM_APPLE)

#endif
#include "RenderAPI.h"
namespace anv
{
	Ref<RenderTarget> anv::RenderTarget::Create(_shared<Context> _ctx,
		RenderTargetType _type, uint32_t _width, uint32_t _height)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			#ifdef PLATFORM_WIN64
			return Ref<VulkanRenderTarget>::Create(_ctx, _type, _width, _height);
			#else
			return nullptr;
			#endif
			break;
		case GraphicsAPI::MTL:
			#ifdef PLATFORM_APPLE
			//return Ref<MetalRenderTarget>::Create(_ctx, _type, _width, _height);
			return nullptr;
			#else
			return nullptr;
			#endif
			break;
		default:
			ANV_LOG_FATAL("No supported render API");
			break;
		}
	}

	anv::RenderTarget::RenderTarget(_shared<Context> _ctx, RenderTargetType _type, uint32_t _width, uint32_t _height)
		: m_Context(_ctx), m_Type(_type), m_Width(_width), m_Height(_height)
	{
	}
}
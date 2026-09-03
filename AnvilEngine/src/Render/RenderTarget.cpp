#include "RenderTarget.h"
#include <Core/Macros.h>
#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
#include "Platform/Vulkan/VulkanRenderTarget.h"
#elif defined(PLATFORM_APPLE)
#include "Platform/Metal/MtlRenderTarget.h"
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
			#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
			return Ref<VulkanRenderTarget>::Create(_ctx, _type, _width, _height);
			#else
			return nullptr;
			#endif
			break;
		case GraphicsAPI::MTL:
			#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
			return Ref<MetalRenderTarget>::Create(
				_ctx,
				_type,
				_width,
				_height);
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
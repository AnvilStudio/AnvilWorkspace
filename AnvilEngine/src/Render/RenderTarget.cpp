#include "RenderTarget.h"

#include "Platform/Vulkan/VulkanRenderTarget.h"
#include "RenderAPI.h"
namespace anv
{
	Ref<RenderTarget> anv::RenderTarget::Create(_shared<Context> _ctx,
		RenderTargetType _type, uint32_t _width, uint32_t _height)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanRenderTarget>::Create(_ctx, _type, _width, _height);
		default:
			ANV_ASSERT(0, "No supported render API");
		}
	}

	anv::RenderTarget::RenderTarget(_shared<Context> _ctx, RenderTargetType _type, uint32_t _width, uint32_t _height)
		: m_Context(_ctx), m_Type(_type), m_Width(_width), m_Height(_height)
	{
	}
}
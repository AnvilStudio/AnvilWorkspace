#include "Framebuffer.h"
#include "RenderAPI.h"
#include "Platform/Vulkan/VulkanFrameBuffer.h"
#include "RenderPass.h"

namespace anv {
	Ref<Framebuffer> Framebuffer::Create(_shared<Context> _ctx, Ref<ImageView> _img_view, Ref<RenderPass> _rp, uint32_t _width, uint32_t _height)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanFrameBuffer>::Create(_ctx, _img_view, _rp, _width, _height);
		default:
			break;
		}
	}

	Framebuffer::Framebuffer(_shared<Context> _ctx, Ref<ImageView> _iv, Ref<RenderPass> _rp, uint32_t _width, uint32_t _height)
		: m_Context(_ctx), m_ImageView(_iv), m_RenderPass(_rp), m_Width(_width), m_Height(_height)
	{
	}
}
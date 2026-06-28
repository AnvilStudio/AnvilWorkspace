#pragma once
#include <cstdint>
#include "Framebuffer.h"
#include "RenderPass.h"
#include "CommandBuffer.h"

namespace anv
{
	enum class RenderTargetType
	{
		RENDER_TARGET_TYPE_IMAGE // texture
	};

	class RenderTarget : public RefCounter
	{
	public:
		ANV_NO_DSCRD
		static Ref<RenderTarget> Create(_shared<Context> _ctx, 
			RenderTargetType _type, uint32_t _width, uint32_t _height);
		
		~RenderTarget() = default;

		virtual RenderTargetType GetType() = 0;
		virtual uint32_t GetWidth() = 0;
		virtual uint32_t GetHeight() = 0;

		virtual void Resize(uint32_t _width, uint32_t _height) = 0;

		virtual Ref<Image2D> GetImage() = 0;
		virtual Ref<RenderPass> GetRenderPass() = 0;
		virtual Ref<Framebuffer> GetFrameBuffer() = 0;

		virtual void Begin(Ref<CommandBuffer> _cmd) = 0;
		virtual void End(Ref<CommandBuffer> _cmd) = 0;

		virtual void* GetImGuiTextureID() = 0;
	protected:
		RenderTarget(_shared<Context> _ctx,
			RenderTargetType _type, uint32_t _width, uint32_t _height);

		_shared<Context> m_Context;
		Ref<Image2D> m_Image;
		Ref<Framebuffer> m_Framebuffer;
		Ref<RenderPass> m_Renderpass;

		RenderTargetType m_Type;

		uint32_t m_Width;
		uint32_t m_Height;
	};
}
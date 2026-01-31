#pragma once
#include "../Core/Reference.h"
#include "Image.h"
#include "Context.h"

namespace anv {

	class RenderPass;

	class Framebuffer
		: public RefCounter
	{
	public:
		static Ref<Framebuffer> Create(_shared<Context> _ctx, Ref<ImageView> _img_view, Ref<RenderPass> _rp);

		Framebuffer(_shared<Context> _ctx, Ref<ImageView> _iv, Ref<RenderPass> _rp);

		virtual ~Framebuffer() = default;

	protected:
		_shared<Context> m_Context;
		Ref<ImageView>   m_ImageView;
		Ref<RenderPass>  m_RenderPass;
	};
}


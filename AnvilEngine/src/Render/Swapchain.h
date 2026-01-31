#pragma once
#include "../Core/Reference.h"
#include "../Util/UMacros.h"
#include "Image.h"

namespace  anv
{
	class Context;

	struct SwapExtent
	{
		uint32_t width;
		uint32_t height;
	};

	class Swapchain
		: public RefCounter
	{
	public:
		ANV_NO_DSCRD
		static Ref<Swapchain> Create(_shared<Context> _ctx);

		Swapchain(_shared<Context> _ctx);
		Swapchain() = default;

		virtual ~Swapchain()           = default;
		virtual SwapExtent GetExtent() = 0;

		// Recreate the swapchain, Images, and Image views
		// Call on window resize
		virtual void ResetSwap() = 0;

		_vec<Ref<ImageView>> GetImageViews() { return m_ImageViews; };

	protected:
		_shared<Context>         m_Context;
		_vec<Ref<ImageView>> m_ImageViews;
		_vec<Ref<Image2D>>    m_Images;
	};
}

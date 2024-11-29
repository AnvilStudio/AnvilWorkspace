#pragma once

#include "../Util/UMacros.h"

namespace  anv
{
	class Context;

	struct SwapExtent
	{
		float width;
		float height;
	};

	class Swapchain
	{
	public:
		static _unique<Swapchain> Create(Context* _ctx);

		virtual void OnDestroy() = 0;
		virtual SwapExtent GetExtent() = 0;
	};
}

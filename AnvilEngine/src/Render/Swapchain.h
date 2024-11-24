#pragma once

#include "../Util/UMacros.h"

namespace  anv
{
	class Context;

	class Swapchain
	{
	public:
		static _unique<Swapchain> Create(Context* _ctx);

		virtual void OnDestroy() = 0;
	};
}

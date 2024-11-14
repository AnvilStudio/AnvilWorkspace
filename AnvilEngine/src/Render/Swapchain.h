#pragma once

#include "../Util/UMacros.h"
#include "Context/Context.h"

namespace  anv
{
	class Swapchain
	{
	public:
		static 
		_unique(Swapchain) Create(Context* _ctx);
	};
}

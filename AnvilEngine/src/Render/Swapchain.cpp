#include "Swapchain.h"
#include <Render/Platform/Vulkan/VulkanSwapChain.h>

namespace anv 
{
	_unique(Swapchain) Create(Context* _ctx)
	{
		return std::make_unique<VulkanSwapChain>(_ctx);
	}
}

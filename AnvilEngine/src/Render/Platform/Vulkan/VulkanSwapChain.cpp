#include "VulkanSwapChain.h"
#include "VulkanContext.h"

namespace anv
{
	VulkanSwapChain::VulkanSwapChain(Context* _ctx)
	{
		dynamic_cast<VulkanContext*>(_ctx);
	}
}
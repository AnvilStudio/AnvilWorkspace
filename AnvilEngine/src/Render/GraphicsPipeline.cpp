#include "GraphicsPipeline.h"
#include "Platform/Vulkan/VulkanPipeline.h"

namespace anv
{
	Ref<GraphicsPipeline> GraphicsPipeline::Create(_shared<Context> _ctx)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanPipeline>::Create(_ctx);
			break;
		}
	}
}
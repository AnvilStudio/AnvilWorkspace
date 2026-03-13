#include "GraphicsPipeline.h"
#include "Platform/Vulkan/VulkanPipeline.h"
#include "RenderAPI.h"

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

	GraphicsPipeline::GraphicsPipeline(_shared<Context> _ctx, std::string _dName)
		: Asset(_dName), m_Context(_ctx) {}
}
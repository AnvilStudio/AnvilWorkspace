#include "GraphicsPipeline.h"
#ifdef PLATFORM_WIN64
#include "Platform/Vulkan/VulkanPipeline.h"
#elif defined(PLATFORM_APPLE)
#endif
#include "RenderAPI.h"

namespace anv
{
	Ref<GraphicsPipeline> GraphicsPipeline::create_pipeline_asset(_shared<Context> _ctx, std::string _dName)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			#ifdef PLATFORM_WIN64
			return Ref<VulkanPipeline>::Create(_ctx, _dName);
			#else
			return nullptr;
			#endif
			break;
		case GraphicsAPI::MTL:
			#ifdef PLATFORM_APPLE
			//return Ref<MetalPipeline>::Create(_ctx, _dName);
			return nullptr;
			#else
			return nullptr;
			#endif
			break;
		}
	}

	GraphicsPipeline::GraphicsPipeline(_shared<Context> _ctx, std::string _dName)
		: Asset(_dName), m_Context(_ctx) {
			m_Type = "Graphics Pipeline";
		}
}
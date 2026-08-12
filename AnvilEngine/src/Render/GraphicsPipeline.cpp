#include "GraphicsPipeline.h"
#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
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
			#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
			return Ref<VulkanPipeline>::Create(_ctx, _dName);
			#else
			return nullptr;
			#endif
			break;
		case GraphicsAPI::MTL:
			#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
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

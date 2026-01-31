#include "CommandBuffer.h"
#include "RenderAPI.h"
#include "Platform/Vulkan/VulkanCommandBuffer.h"

namespace anv
{
	Ref<CommandBuffer> CommandBuffer::Create(_shared<Context> _ctx)
	{
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanCommandBuffer>::Create(_ctx);
		}
	}

	CommandBuffer::CommandBuffer(_shared<Context> _ctx)
		: m_Context(_ctx)
	{
		
	}
}
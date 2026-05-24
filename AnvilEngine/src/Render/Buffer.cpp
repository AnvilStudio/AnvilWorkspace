#include "Buffer.h"
#include "Platform/Vulkan/VulkanBuffer.h"

namespace anv
{
	Buffer::Buffer(_shared<Context> _ctx)
		: m_Context(_ctx)
	{
		ANV_LOG_INFO("Creating Buffer");
	}

	template<typename T>
	inline Ref<Buffer> anv::Buffer::Create(_shared<Context> _ctx, BufferCreateInfo _info)
	{
		// TODO: API switch when adding multiple render APIs
		return Ref<VulkanBuffer>::Create(_ctx, _info)
	}
}
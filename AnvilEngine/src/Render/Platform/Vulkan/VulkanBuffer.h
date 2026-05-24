#pragma once
#include "../../Buffer.h"
#include "vulkan/vulkan.h"

namespace anv
{
	class VulkanBuffer : public Buffer
	{
	public:
		VulkanBuffer(_shared<Context> _ctx, BufferCreateInfo _info);
		~VulkanBuffer();

		virtual void SetData(const void* data, uint64_t size, uint64_t offset = 0) override;
		virtual void Bind() override;

		virtual uint64_t GetSize() const override;
		virtual BufferUsage GetUsage() const override;

		VkBuffer GetBuffer() { return m_Buffer; }
	private:
		void set_vk_usage();
	private:
		BufferCreateInfo m_Info{};

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;

		VkBufferUsageFlags m_VkUsage = 0;
		VkMemoryPropertyFlags m_Properties = 0;
	};
}
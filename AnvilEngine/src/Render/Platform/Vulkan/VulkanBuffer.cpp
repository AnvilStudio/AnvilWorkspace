#include "VulkanBuffer.h"
#include "VulkanUtil.h"
#include <Render/Context.h>
#include "VulkanContext.h"
namespace anv
{
	VulkanBuffer::VulkanBuffer(_shared<Context> _ctx, BufferCreateInfo _info)
		: Buffer(_ctx)
	{
		set_vk_usage();

		m_Properties =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		vk_util::create_buffer(
			m_Context->GetAs<VulkanContext>()->GetPhysicalDevice(),
			m_Context->GetAs<VulkanContext>()->GetDevice(),
			m_Info.Size,
			m_VkUsage,
			m_Properties,
			m_Buffer,
			m_Memory
		);

		if (m_Info.InitialData)
			SetData(m_Info.InitialData, m_Info.Size, 0);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		if (!m_Context)
			return;

		VkDevice device = m_Context->GetAs<VulkanContext>()->GetDevice();

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}
	}

	void VulkanBuffer::SetData(const void* data, uint64_t size, uint64_t offset)
	{
		ANV_ASSERT(data, "VulkanBuffer::SetData data is null!");
		ANV_ASSERT(offset + size <= m_Info.Size, "Buffer upload exceeds buffer size!");

		void* mapped = nullptr;

		vkMapMemory(
			m_Context->GetAs<VulkanContext>()->GetDevice(),
			m_Memory,
			offset,
			size,
			0,
			&mapped
		);

		memcpy(mapped, data, size);

		vkUnmapMemory(
			m_Context->GetAs<VulkanContext>()->GetDevice(),
			m_Memory
		);
	}

	void VulkanBuffer::Bind()
	{
	}

	uint64_t VulkanBuffer::GetSize() const
	{
		m_Info.Size;
	}

	BufferUsage VulkanBuffer::GetUsage() const
	{
		return m_Info.Usage;
	}

	void VulkanBuffer::set_vk_usage()
	{
		switch (m_Info.Usage)
		{
		case BufferUsage::Vertex:
			m_VkUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			break;

		case BufferUsage::Index:
			m_VkUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			break;

		case BufferUsage::Uniform:
			m_VkUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			break;

		default:
			ANV_LOG_FATAL("Unknown buffer usage!");
			break;
		}
	}

}
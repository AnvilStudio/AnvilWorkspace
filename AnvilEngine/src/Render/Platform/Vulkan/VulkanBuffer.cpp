#include "VulkanBuffer.h"
#include "VulkanUtil.h"
#include <Render/Context.h>
#include "VulkanContext.h"
namespace anv
{
	VulkanBuffer::VulkanBuffer(_shared<Context> _ctx, BufferCreateInfo _info)
		: Buffer(_ctx), m_Info(_info)
	{
		ANV_ASSERT(m_Info.Size > 0, "Cannot create VulkanBuffer with size 0!");
		ANV_LOG_INFO("Creating VK buffer with size: %i", m_Info.Size);

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
			m_Memory);

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

	void VulkanBuffer::SetData(
		const void *data,
		uint64_t size,
		uint64_t offset)
	{
		ANV_ASSERT(data, "Buffer data is null!");
		ANV_ASSERT(
			offset <= m_Info.Size &&
				size <= m_Info.Size - offset,
			"Buffer upload exceeds capacity!");

		VkDevice device =
			m_Context->GetAs<VulkanContext>()->GetDevice();

		void *mapped = nullptr;

		ANV_ASSERT(
			vkMapMemory(
				device,
				m_Memory,
				0,
				VK_WHOLE_SIZE,
				0,
				&mapped) == VK_SUCCESS,
			"Failed to map Vulkan buffer!");

		std::memcpy(
			static_cast<std::byte *>(mapped) + offset,
			data,
			static_cast<size_t>(size));

		vkUnmapMemory(device, m_Memory);
	}

	void VulkanBuffer::Bind()
	{
	}

	uint64_t VulkanBuffer::GetSize() const
	{
		return m_Info.Size;
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
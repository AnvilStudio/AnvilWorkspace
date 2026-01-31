#include "VulkanCommandBuffer.h"

namespace anv
{
	VulkanCommandBuffer::VulkanCommandBuffer(_shared<Context> _ctx)
		: CommandBuffer(_ctx)
	{
		auto vk = m_Context->GetAs<VulkanContext>();

		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.commandBufferCount = 1;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandPool = vk->GetCommandPool();

		ANV_VK_CHECK_RESULT(vkAllocateCommandBuffers(vk->GetDevice(), &info, &m_CmdBuffer), "Failed to allocate VK Command Buffer")
	}

	void VulkanCommandBuffer::Begin()  {
		VkCommandBufferBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags = 0;
		info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(m_CmdBuffer, &info);
	}

	void VulkanCommandBuffer::End()    {
		vkEndCommandBuffer(m_CmdBuffer);
	}

	void anv::VulkanCommandBuffer::Submit(VkSemaphore _vkWaitSemaphore, VkSemaphore _vkSignalSemaphore, VkFence _vkFence) {
		VkSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// Wait: image is available for rendering
		VkPipelineStageFlags waitStages[] = {
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &_vkWaitSemaphore;
		info.pWaitDstStageMask = waitStages;

		// Cmd buffer
		info.commandBufferCount = 1;
		info.pCommandBuffers = &m_CmdBuffer;

		// Signal: rendering finished (present will wait on this)
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &_vkSignalSemaphore;

		VkQueue graphicsQueue = m_Context->GetAs<VulkanContext>()->GetGraphicsQueue();

		vkQueueSubmit(graphicsQueue, 1, &info, _vkFence);
		
	}

	void VulkanCommandBuffer::Reset()  {
		vkResetCommandBuffer(m_CmdBuffer, 0);
	}
}
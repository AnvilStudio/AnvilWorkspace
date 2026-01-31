#pragma once
#include "../../Swapchain.h"
#include "VulkanUtil.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace anv
{
	class Context;

	class VulkanContext;

	class VulkanSwapchain
		: public Swapchain
	{
	public:
		VulkanSwapchain(_shared<Context> _ctx);
		VulkanSwapchain() = default;
		virtual ~VulkanSwapchain() override;

		VkFormat   GetFormat() { return m_ImageFormat; }
		SwapExtent GetExtent() override
		{
			return {m_Extent.width, m_Extent.height};
		}
		void Reset() override;
		void OnDestroy(VkDevice _dev);

		_vec<Ref<Image2D>> GetImages() { return m_Images; }

		uint32_t AcquireNextImage(VkSemaphore imageAvailable, bool& swap_recreate, VkFence fence = VK_NULL_HANDLE);
		void Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore renderFinished, bool& swap_recreate);
		VkSwapchainKHR GetHandle() const { return m_Swapchain; }


	private:
		void query_support();
		void create_vk_swapchain();
		void create_vk_img_views();
	private:
		vk_util::SwapchainSupportDetails m_SupportDetails;

		VkSwapchainKHR    m_Swapchain;
		VkFormat          m_ImageFormat;
		VkExtent2D        m_Extent;
	};
}


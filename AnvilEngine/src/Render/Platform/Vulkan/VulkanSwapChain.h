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

	private:
		void query_support();
		void create_vk_swapchain();

	private:
		vk_util::SwapchainSupportDetails m_SupportDetails;

		VkSwapchainKHR    m_Swapchain;
		VkFormat          m_ImageFormat;
		VkExtent2D        m_Extent;
	};
}


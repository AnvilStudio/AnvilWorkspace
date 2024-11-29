#pragma once
#include "../../Swapchain.h"
#include "VulkanContext.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace anv
{
	class Context;

	class VulkanSwapchain
		: public Swapchain
	{
	public:
		VulkanSwapchain(Context* _ctx);
		~VulkanSwapchain();

		VkFormat   GetFormat() { return m_ImageFormat; }
		SwapExtent GetExtent() override
		{
			return {(float)m_Extent.width, (float)m_Extent.height};
		}

		void OnDestroy() override;

	private:
		void querey_support();
		void create_vk_swapchain();
		void create_image_views();

	private:
		vk_util::
		SwapchainSupportDetails m_SupportDetails;

		VulkanContext*    m_VkContext;
		VkSwapchainKHR    m_Swapchain;
		_vec<VkImage>     m_SwapchainImages;
		_vec<VkImageView> m_ImageViews;
		VkFormat          m_ImageFormat;
		VkExtent2D        m_Extent;
	};
}


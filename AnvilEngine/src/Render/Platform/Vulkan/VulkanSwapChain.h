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
		VkExtent2D GetExtent() { return m_Extent; }


	private:
		void querey_support();
		void create_vk_swapchain();

	private:
		vk_util::
		SwapchainSupportDetails m_SupportDetails;

		VulkanContext* m_VkContext;
		VkSwapchainKHR m_Swapchain;
		_vec<VkImage>  m_SwapchainImages;
		VkFormat       m_ImageFormat;
		VkExtent2D     m_Extent;
	};
}


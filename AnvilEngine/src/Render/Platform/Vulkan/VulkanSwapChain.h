#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Render/Swapchain.h>

namespace anv
{

	class VulkanSwapChain
		: public Swapchain
	{
	public:
		VulkanSwapChain(Context* _ctx);
		~VulkanSwapChain();

	private:
		VkSwapchainKHR m_Swapchain;
	};
}


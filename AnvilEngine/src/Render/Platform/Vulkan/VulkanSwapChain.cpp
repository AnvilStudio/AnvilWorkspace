#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"
#include "Core/App.h"

namespace anv
{
	VulkanSwapchain::VulkanSwapchain(Context* _ctx)
	{
		ANV_PROFILE_SCOPE()
		m_VkContext = _ctx->GetNativeContextAs<VulkanContext>();
		querey_support();
		create_vk_swapchain();
		create_image_views();
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		vkDestroySwapchainKHR(m_VkContext->GetDevice(), m_Swapchain, nullptr);
	}

	void VulkanSwapchain::OnDestroy()
	{
		for (auto iView : m_ImageViews)
		{
			vkDestroyImageView(m_VkContext->GetDevice(), iView, nullptr);
		}
	}

	void VulkanSwapchain::querey_support()
	{
			m_SupportDetails =
			vk_util::vku_QuerySwapChainSupport(m_VkContext->GetPhysicalDevice(), m_VkContext->GetSurface());
	}

	void VulkanSwapchain::create_vk_swapchain()
	{
		VkSurfaceFormatKHR surfaceFormat = vk_util::vku_ChooseSwapSurfaceFormat(m_SupportDetails.formats);
		VkPresentModeKHR presentMode = vk_util::vku_ChooseSwapPresentMode(m_SupportDetails.presentModes);
		VkExtent2D extent = vk_util::vku_ChooseSwapExtent(m_SupportDetails.capabilities,
			m_VkContext->GetWinHandle());

		uint32_t imageCount = m_SupportDetails.capabilities.minImageCount + 1;
		if (m_SupportDetails.capabilities.maxImageCount > 0 && imageCount > m_SupportDetails.capabilities.maxImageCount) {
			imageCount = m_SupportDetails.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_VkContext->GetSurface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		vk_util::QueueFamilyIndices indices = vk_util::vku_FindQueueFamilies(m_VkContext->GetPhysicalDevice(),
			m_VkContext->GetSurface());
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
		createInfo.preTransform = m_SupportDetails.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		ANV_VK_CHECK_RESULT(vkCreateSwapchainKHR(m_VkContext->GetDevice(), &createInfo, nullptr, &m_Swapchain),
			"Failed to create Swapchain!")

		// Create Frame Buffers
		m_SwapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_VkContext->GetDevice(), m_Swapchain, &imageCount, m_SwapchainImages.data());
		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;
	}

	void VulkanSwapchain::create_image_views()
	{
		m_ImageViews.resize(m_SwapchainImages.size());

		// create image view for every image
		// NOTE: could make images and image views a vec of std::pair<VkImage, VkImageView>
		for (size_t i = 0; i < m_SwapchainImages.size(); i++)
		{
			VkImageViewCreateInfo ivInfo{};
			ivInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			ivInfo.image = m_SwapchainImages[i];
			ivInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			ivInfo.format = m_ImageFormat;

			// R, G, B, A channel routing
			ivInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ivInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// describe pourpose, what part of the image to access
			// color target, no mipmap levels, one layer
			ivInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			ivInfo.subresourceRange.baseMipLevel = 0;
			ivInfo.subresourceRange.levelCount = 1;
			ivInfo.subresourceRange.baseArrayLayer = 0;
			ivInfo.subresourceRange.layerCount = 1;

			ANV_VK_CHECK_RESULT(vkCreateImageView(m_VkContext->GetDevice(), &ivInfo, nullptr, &m_ImageViews[i]), "Failed to create image view!");
		}
	}
}
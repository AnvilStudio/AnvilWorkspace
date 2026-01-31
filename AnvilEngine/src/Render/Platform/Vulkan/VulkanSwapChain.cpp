#include "VulkanSwapChain.h"
#include "VulkanContext.h"
#include "VulkanUtil.h"
#include "Core/App.h"
#include "VulkanImage.h"

namespace anv
{
	VulkanSwapchain::VulkanSwapchain(_shared<Context> _ctx)
		: Swapchain(_ctx)
	{
		ANV_PROFILE_SCOPE()
		query_support();
		create_vk_swapchain();
		create_vk_img_views();
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		ANV_PROFILE_SCOPE()
		for (auto& img : m_ImageViews)
		{
			img->OnDestroy();
		}
		//vkDeviceWaitIdle(m_Context->GetAs<VulkanContext>()->GetDevice());
		auto dev = m_Context->GetAs<VulkanContext>()->GetDevice();
		vkDestroySwapchainKHR(dev, m_Swapchain, nullptr);
	}


	void VulkanSwapchain::ResetSwap()
	{
		ANV_PROFILE_SCOPE();

		auto dev = m_Context->GetAs<VulkanContext>()->GetDevice();
		vkDeviceWaitIdle(dev);

		for (auto& img : m_ImageViews)
			img->OnDestroy();
		m_ImageViews.clear();

		vkDestroySwapchainKHR(dev, m_Swapchain, nullptr);
		m_Swapchain = VK_NULL_HANDLE;

		query_support();
		create_vk_swapchain();
		create_vk_img_views();
	}


	void VulkanSwapchain::OnDestroy(VkDevice _dev)
	{
		ANV_PROFILE_SCOPE()
		for (auto& img : m_ImageViews)
		{
			img->OnDestroy();
		}
		vkDeviceWaitIdle(m_Context->GetAs<VulkanContext>()->GetDevice());
		vkDestroySwapchainKHR(m_Context->GetAs<VulkanContext>()->GetDevice(), m_Swapchain, nullptr);
	}

	void VulkanSwapchain::query_support()
	{
		m_SupportDetails = vk_util::vku_QuerySwapChainSupport(
			m_Context->GetAs<VulkanContext>()->GetPhysicalDevice(), 
			m_Context->GetAs<VulkanContext>()->GetSurface()
		);
	}

	void VulkanSwapchain::create_vk_swapchain()
	{
		VkSurfaceFormatKHR surfaceFormat = vk_util::vku_ChooseSwapSurfaceFormat(m_SupportDetails.formats);
		VkPresentModeKHR presentMode = vk_util::vku_ChooseSwapPresentMode(m_SupportDetails.presentModes);
		VkExtent2D extent = vk_util::vku_ChooseSwapExtent(
			m_SupportDetails.capabilities,
			m_Context->GetAs<VulkanContext>()->GetWinHandle()
		);

		uint32_t imageCount = m_SupportDetails.capabilities.minImageCount + 1;
		if (m_SupportDetails.capabilities.maxImageCount > 0 && imageCount > m_SupportDetails.capabilities.maxImageCount) {
			imageCount = m_SupportDetails.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Context->GetAs<VulkanContext>()->GetSurface();

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		vk_util::QueueFamilyIndices indices = vk_util::vku_FindQueueFamilies(
			m_Context->GetAs<VulkanContext>()->GetPhysicalDevice(),
			m_Context->GetAs<VulkanContext>()->GetSurface()
		);

		uint32_t queueFamilyIndices[] = { 
			indices.graphicsFamily.value(), indices.presentFamily.value()
		};

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

		ANV_VK_CHECK_RESULT(vkCreateSwapchainKHR(
			m_Context->GetAs<VulkanContext>()->GetDevice(), &createInfo, nullptr, &m_Swapchain),
			"Failed to create Swapchain!"
		)

		m_Images.resize(imageCount);
		_vec<VkImage> imgs(imageCount);

		// Retrieve the swapchain images
		vkGetSwapchainImagesKHR(
			m_Context->GetAs<VulkanContext>()->GetDevice(), 
			m_Swapchain, &imageCount, imgs.data()
		);

		// Create the image2D objects
		for (int i = 0; i < m_Images.size(); i++)
		{
			m_Images[i] = Ref<VulkanImage2D>::Create(
				m_Context, 
				imgs[i],
				vk_util::vku_ToEngineImgFormat(surfaceFormat.format)
			);
		}

		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;
	}

	uint32_t anv::VulkanSwapchain::AcquireNextImage(VkSemaphore _imageAvailable, bool& _swapRecreate, VkFence _fence)
	{
		uint32_t imageIndex = 0;
		auto dev = m_Context->GetAs<VulkanContext>()->GetDevice();

		VkResult r = vkAcquireNextImageKHR(
			dev,
			m_Swapchain,
			UINT64_MAX,
			_imageAvailable,
			_fence,
			&imageIndex
		);

		// Simple now: treat out-of-date as "needs reset" and bail
		if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
			_swapRecreate = true;
			return 0;
		}

		return imageIndex;
	}

	void VulkanSwapchain::Present(VkQueue presentQueue, uint32_t imageIndex, VkSemaphore renderFinished, bool& swap_recreate)
	{
		VkPresentInfoKHR pi{};
		pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		pi.waitSemaphoreCount = 1;
		pi.pWaitSemaphores = &renderFinished;

		VkSwapchainKHR sc = m_Swapchain;
		pi.swapchainCount = 1;
		pi.pSwapchains = &sc;
		pi.pImageIndices = &imageIndex;

		VkResult r = vkQueuePresentKHR(presentQueue, &pi);
		if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
			swap_recreate = true;
			return;
		}
	}

	void VulkanSwapchain::create_vk_img_views()
	{
		m_ImageViews.resize(m_Images.size());

		for (size_t i = 0; i < m_Images.size(); i++)
		{
			// VulkanImage2D::MakeImageView() returns Ref<ImageView>
			m_ImageViews[i] = m_Images[i].As<VulkanImage2D>()->MakeImageView();
		}
	}
}
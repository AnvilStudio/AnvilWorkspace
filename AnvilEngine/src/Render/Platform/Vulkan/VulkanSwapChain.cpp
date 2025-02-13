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


	// TODO: IMPL BETTER!!!
	void VulkanSwapchain::Reset() 
	{
		ANV_LOG_WARN("Swapchain::Reset() Is not yet impl properly, use caution...")
		ANV_PROFILE_SCOPE()

		delete m_Swapchain;
		m_Swapchain = nullptr;
		create_vk_swapchain();
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

}
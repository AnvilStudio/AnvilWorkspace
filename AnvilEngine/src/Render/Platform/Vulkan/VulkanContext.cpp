#include "VulkanContext.h"
#include "Core/Window.h"
#include "Core/App.h"
#include "VulkanSwapchain.h"

#include <set>

namespace anv {
	VulkanContext::VulkanContext(Window* _win)
		: Context(_win)
	{
		ANV_PROFILE_SCOPE();

		vkc_instance  (); // instance creation
		vkc_surface   (); // rendering surface
		vkc_physical  (); // select gpu
		vkc_logical   (); // create logical device
		vkc_cmd_pool  ();
	}

	VulkanContext::~VulkanContext()
	{
		ANV_PROFILE_SCOPE();


		m_Swapchain.Reset();

		//if (m_CmdPool != VK_NULL_HANDLE)
		//{
		//	vkDestroyCommandPool(m_Device, m_CmdPool, nullptr);
		//}

		vkDestroyDevice(m_Device, nullptr);

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

#ifdef DEBUG
		// Destroy the Debug messenger
		m_DebugInfo.DestroyDebugUtilsMessengerEXT(m_Instance, nullptr);
#endif

		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanContext::CreateSwapchain()
	{
		if (m_Swapchain)
		{
			m_Swapchain.Reset();
		}

		m_Swapchain = Ref<VulkanSwapchain>::Create(shared_from_this());
	}

	// == Privates

	void VulkanContext::vkc_instance()
	{
		ANV_PROFILE_SCOPE();
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.apiVersion = VK_API_VERSION_1_0;
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pApplicationName = "Anvil";
		appInfo.pEngineName = "Anvil";

		VkInstanceCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
		};

	// Get Required Extensions

		auto extensions = vk_util::vku_GetRequiredExtensions();
		
		#ifdef __APPLE__
		// Required on macOS with MoltenVK
		info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

		#endif
		info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		info.ppEnabledExtensionNames = extensions.data();


#ifdef DEBUG 

	// Print Available Extensions
	{   
		// Query the number of available extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		// Allocate a vector to hold extension properties
		_vec<VkExtensionProperties> exten(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, exten.data());

		// Print the available extensions
		ANV_LOG_INFO("Available Vulkan Extensions:")
		for (const auto& ext : exten) {
			std::string e = "\t" + std::string(ext.extensionName) + " (Version: " + std::to_string(ext.specVersion) + ")";
			ANV_LOG_INFO(e)
		}
	}

	// Validation Layers //

		if (!vk_util::vku_CheckValidationSupport())
			ANV_LOG_ERROR("Vulkan: Validation layers requested but not supported!");

		info.enabledLayerCount = static_cast<uint32_t>(m_DebugInfo.Layers.size());
		info.ppEnabledLayerNames = m_DebugInfo.Layers.data();

		// Set up debug messenger create info
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
#ifdef DEBUG_G

		// add api dump layer
		info.enabledLayerCount = static_cast<uint32_t>(m_DebugInfo.Layers.size()) + 1;
		m_DebugInfo.Layers.push_back("VK_LAYER_LUNARG_api_dump");
		info.ppEnabledLayerNames = m_DebugInfo.Layers.data();

		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
#else
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
#endif
		debugCreateInfo.pfnUserCallback = vk_util::VKDebugInfo::DebugCallback;

		info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else  // RELEASE
		info.enabledLayerCount = 0;
#endif 

		ANV_VK_CHECK_RESULT(vkCreateInstance(&info, nullptr, &m_Instance), 
			"Failed to create a vulkan instance!")

	// Validation Messenger
#ifdef DEBUG
		 ANV_VK_CHECK_RESULT(m_DebugInfo.CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr), 
			 "Failed to create VK validation messenger!")
#endif // DEBUG

	}

#ifdef DEBUG

#endif // DEBUG

	void VulkanContext::vkc_surface()
	{
		ANV_VK_CHECK_RESULT(glfwCreateWindowSurface(m_Instance, m_WinHandle, nullptr, &m_Surface),
			"Failed to create a window surface!")
	}
		
	void VulkanContext::vkc_physical()
	{
		ANV_PROFILE_SCOPE();

		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			ANV_LOG_FATAL("Vulkan: Failed to find a suitable gpu!");

		_vec<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		m_PhysicalDevice = vk_util::vku_FindSuitableDevice(devices, m_Surface, m_DeviceExtensions);
	}

	void VulkanContext::vkc_logical()
	{
		ANV_PROFILE_SCOPE();

		m_QueueFamilies = vk_util::vku_FindQueueFamilies(m_PhysicalDevice, m_Surface);

		// Graphics & Present Queues //
		_vec<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilies.graphicsFamily.value(), m_QueueFamilies.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};

		#ifdef __APPLE__
			m_DeviceExtensions.push_back("VK_KHR_portability_subset");
		#endif

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

#ifdef DEBUG
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_DebugInfo.Layers.size());
		createInfo.ppEnabledLayerNames = m_DebugInfo.Layers.data();
#else
		createInfo.enabledLayerCount = 0;
#endif
		ANV_VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device),
			"Failed to create a logical device!")
		
		// Retreve queue handles
		vkGetDeviceQueue(m_Device, m_QueueFamilies.graphicsFamily.value(), 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_QueueFamilies.presentFamily.value(), 0, &m_PresentQueue);
	}

	void VulkanContext::vkc_cmd_pool()
	{
		ANV_PROFILE_SCOPE();

		vk_util::QueueFamilyIndices qFamilyIndices = vk_util::vku_FindQueueFamilies(m_PhysicalDevice, m_Surface);

		VkCommandPoolCreateInfo cpinfo{};
		cpinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cpinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		cpinfo.queueFamilyIndex = qFamilyIndices.graphicsFamily.value();

		ANV_VK_CHECK_RESULT(vkCreateCommandPool(m_Device, &cpinfo, nullptr, &m_CmdPool),
			"Failed to create a VkCommandPool!");
	}

	void VulkanContext::create_immediate_submit_objects()
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags =
			VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex =
			GetQueueFamilies().graphicsFamily.value();

		ANV_VK_CHECK_RESULT(
			vkCreateCommandPool(
				m_Device,
				&poolInfo,
				nullptr,
				&m_ImmediateCommandPool
			),
			"Failed to create immediate command pool!"
		);

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType =
			VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		ANV_VK_CHECK_RESULT(
			vkCreateFence(
				m_Device,
				&fenceInfo,
				nullptr,
				&m_ImmediateFence
			),
			"Failed to create immediate fence!"
		);
	}

	// Bypass the renderers queue and submit taskings on demand
	void VulkanContext::ImmediateSubmit(
		const ImmediateSubmitFn& fn)
	{
		vkResetFences(
			m_Device,
			1,
			&m_ImmediateFence
		);

		vkResetCommandPool(
			m_Device,
			m_ImmediateCommandPool,
			0
		);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool =
			m_ImmediateCommandPool;
		allocInfo.level =
			VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer cmd;

		ANV_VK_CHECK_RESULT(
			vkAllocateCommandBuffers(
				m_Device,
				&allocInfo,
				&cmd
			),
			"Failed to allocate immediate command buffer!"
		);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags =
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		ANV_VK_CHECK_RESULT(
			vkBeginCommandBuffer(
				cmd,
				&beginInfo
			),
			"Failed to begin immediate command buffer!"
		);

		fn(cmd);

		ANV_VK_CHECK_RESULT(
			vkEndCommandBuffer(cmd),
			"Failed to end immediate command buffer!"
		);

		VkSubmitInfo submitInfo{};
		submitInfo.sType =
			VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers =
			&cmd;

		ANV_VK_CHECK_RESULT(
			vkQueueSubmit(
				m_GraphicsQueue,
				1,
				&submitInfo,
				m_ImmediateFence
			),
			"Failed immediate queue submit!"
		);

		vkWaitForFences(
			m_Device,
			1,
			&m_ImmediateFence,
			VK_TRUE,
			UINT64_MAX
		);

		vkFreeCommandBuffers(
			m_Device,
			m_ImmediateCommandPool,
			1,
			&cmd
		);
	}

	void VulkanContext::destroy_immediate_submit_objects()
	{
		if (m_ImmediateFence)
			vkDestroyFence(
				m_Device,
				m_ImmediateFence,
				nullptr
			);

		if (m_ImmediateCommandPool)
			vkDestroyCommandPool(
				m_Device,
				m_ImmediateCommandPool,
				nullptr
			);

		m_ImmediateFence = VK_NULL_HANDLE;
		m_ImmediateCommandPool = VK_NULL_HANDLE;
	}
	
	void VulkanContext::IdleDevice()
	{
		vkDeviceWaitIdle(m_Device);
	}
}


#include "VulkanUtil.h"
#include "VulkanContext.h"
#include <map>
#include <set>

namespace anv::vk_util
{
	VkResult VKCDebugInfo::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        return func ? func(instance, pCreateInfo, pAllocator, &debugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }


    void VKCDebugInfo::DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(instance, debugMessenger, pAllocator);
    }


    _vec(const char*) VKUtil_GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // DEBUG

        return extensions;
    }


    bool VKUtil_CheckValidationSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        _vec(VkLayerProperties) availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : VKCDebugInfo::Layers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
    }

    bool VkUtil_CheckDeviceExtensionSupport(VkPhysicalDevice _device, const _vec(const char*) _extensions)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(_extensions.begin(), _extensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    VkPhysicalDevice VKUtil_FindSuitableDevice(_vec(VkPhysicalDevice) _devices, VkSurfaceKHR _surface, const _vec(const char*) _extensions)
    {
        std::map<int, VkPhysicalDevice> gpu_map{};

        for (auto& device : _devices)
        {
            int score = 0;
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            }

            score += deviceProperties.limits.maxImageDimension2D;

            if (!deviceFeatures.geometryShader) {
                score = -1; // Geometry shader is needed
                continue;
            }

            QueueFamilyIndices indices = VKUtil_FindQueueFamilies(device, _surface);

            if (!indices.isComplete())
            {
                score = -1; // Cannot continue without a graphics queue
                continue;
            }

            bool ext_supported = VkUtil_CheckDeviceExtensionSupport(device, _extensions);
            if (!ext_supported)
            {
                score = -1; // swap chain extension needs support
                continue;
            }

            bool swapChainAdequate = false;

            SwapChainSupportDetails swapChainSupport = VkUtil_QuerySwapChainSupport(device, _surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

            if (!swapChainAdequate)
            {
                score = -1; // swap chain needs support
                continue;
            }

            gpu_map[score] = device;
            std::cout << "GPU: " << deviceProperties.deviceName << "\n\tScore: " << score << '\n';
        }

        int final = 0;

        for (auto& score : gpu_map)
        {
            if (score.first > final)
                final = score.first;
            
        }

        return gpu_map[final];
    }

    QueueFamilyIndices VKUtil_FindQueueFamilies(VkPhysicalDevice _device, VkSurfaceKHR _surface)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

        _vec(VkQueueFamilyProperties) queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(_device, i, _surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    SwapChainSupportDetails VkUtil_QuerySwapChainSupport(VkPhysicalDevice _device, VkSurfaceKHR _surface)
    {
        SwapChainSupportDetails details;


        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_device, _surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(_device, _surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) 
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(_device, _surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VKUtil_ChooseSwapSurfaceFormat(const _vec(VkSurfaceFormatKHR) _availableFormats)
    {
        for (const auto& availableFormat : _availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        // else
        return _availableFormats[0];
    }
}
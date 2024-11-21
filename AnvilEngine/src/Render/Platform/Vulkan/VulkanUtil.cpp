
#include "VulkanUtil.h"
#include "VulkanContext.h"
#include "VulkanSwapChain.h"
#include <map>
#include <set>
#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

namespace anv::vk_util
{
	VkResult VKDebugInfo::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        return func ? func(instance, pCreateInfo, pAllocator, &debugMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }


    void VKDebugInfo::DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func) func(instance, debugMessenger, pAllocator);
    }


    _vec<const char*> vku_GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        _vec<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // DEBUG

        return extensions;
    }


    bool vku_CheckValidationSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        _vec<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : VKDebugInfo::Layers) {
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

    bool vku_CheckDeviceExtensionSupport(VkPhysicalDevice _device, const _vec<const char*> _extensions)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, nullptr);

        _vec<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(_device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(_extensions.begin(), _extensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    VkPhysicalDevice vku_FindSuitableDevice(_vec<VkPhysicalDevice> _devices, VkSurfaceKHR _surface, const _vec<const char*> _extensions)
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

            QueueFamilyIndices indices = vku_FindQueueFamilies(device, _surface);

            if (!indices.isComplete())
            {
                score = -1; // Cannot continue without a graphics queue
                continue;
            }

            bool ext_supported = vku_CheckDeviceExtensionSupport(device, _extensions);
            if (!ext_supported)
            {
                score = -1; // swap chain extension needs support
                continue;
            }

            bool swapChainAdequate = false;

            SwapchainSupportDetails swapChainSupport = vku_QuerySwapChainSupport(device, _surface);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

            if (!swapChainAdequate)
            {
                score = -1; // swap chain needs support
                continue;
            }

            gpu_map[score] = device;
            std::string gpu_info = "GPU: " + std::string(deviceProperties.deviceName) + "\n\tScore: " + std::to_string(score);
            ANV_LOG_INFO(gpu_info)
        }

        int final = 0;

        for (auto& score : gpu_map)
        {
            if (score.first > final)
                final = score.first;
            
        }

        return gpu_map[final];
    }

    QueueFamilyIndices vku_FindQueueFamilies(VkPhysicalDevice _device, VkSurfaceKHR _surface)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(_device, &queueFamilyCount, nullptr);

        _vec<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
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

    SwapchainSupportDetails vku_QuerySwapChainSupport(VkPhysicalDevice _device, VkSurfaceKHR _surface)
    {
        SwapchainSupportDetails details;


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

    VkSurfaceFormatKHR vku_ChooseSwapSurfaceFormat(const _vec<VkSurfaceFormatKHR>& _availableFormats)
    {
        for (const auto& availableFormat : _availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        // else
        return _availableFormats[0];
    }

    VkPresentModeKHR vku_ChooseSwapPresentMode(const _vec<VkPresentModeKHR>& _availablePresentModes) {
        for (const auto& availablePresentMode : _availablePresentModes) {
            // tripple buffering support > FIFO
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D vku_ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& _capabilities, GLFWwindow* _window) {
        if (_capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return _capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width  = std::clamp(actualExtent.width,  _capabilities.minImageExtent.width,  _capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, _capabilities.minImageExtent.height, _capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}
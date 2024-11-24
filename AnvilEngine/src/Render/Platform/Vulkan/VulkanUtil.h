#pragma once

#include "Util/UMacros.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

// validates that a VK API call returns VK_SUCCESS. otherwise, throws an error
#define ANV_VK_CHECK_RESULT(f, msg) {                                                            \
    VkResult res = (f);                                                                          \
    if (res != VK_SUCCESS) {                                                                     \
            auto fmt = std::string("Vulkan Error: ") + std::to_string(res) +                     \
            "\nFile: " + __FILE__ +                                                              \
            "\nFunction: " + __FUNCTION__ +                                                      \
            "\nLine: " + std::to_string(__LINE__) +                                              \
            std::string(" : ") + std::string(msg);                                               \
            ANV_LOG_FATAL("== VK CHECK RESULT FAILED ==\n%s", fmt.c_str())                       \
                                                                                                 \
    }                                                                                            \
    else                                                                                         \
    {                                                                                            \
        ANV_LOG_INFO("[%s]: VK CHECK RESULT PASSED", __FUNCTION__)                               \
    }                                                                                            \
}                                                                                                \

namespace anv
{

namespace vk_util {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();;
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct VKDebugInfo
    {
        VkDebugUtilsMessengerEXT debugMessenger;

        // Validation Layers
        inline static _vec<const char*> Layers = {
            "VK_LAYER_KHRONOS_validation",
        };

        inline static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

            switch (messageSeverity)
            {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                ANV_LOG_INFO("== VK VERBOSE ==\n%s", pCallbackData->pMessage)
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                ANV_LOG_INFO("== VK INFO ==\n%s", pCallbackData->pMessage)
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                ANV_LOG_WARN("== VK VALIDATION LAYER WARNING ==\n%s", pCallbackData->pMessage)
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                ANV_LOG_ERROR("== VK VALIDATION LAYER ERROR ==\n%s", pCallbackData->pMessage)
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
                break;
            default:
                break;
            }
            return VK_FALSE;
        }

        VkResult CreateDebugUtilsMessengerEXT
        (VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
            const VkAllocationCallbacks* pAllocator);

        void DestroyDebugUtilsMessengerEXT
        (VkInstance instance, const VkAllocationCallbacks* pAllocator);
    };

    // Retrieves the list of required Vulkan instance extensions.
    _vec<const char*> 
        vku_GetRequiredExtensions();

    // Check if this device supports validation layers
    bool 
        vku_CheckValidationSupport();

    bool 
        vku_CheckDeviceExtensionSupport
        (VkPhysicalDevice _device, const _vec<const char*> _extensions);

    VkPhysicalDevice 
        vku_FindSuitableDevice
        (_vec<VkPhysicalDevice> _devices, 
        VkSurfaceKHR _surface, const _vec<const char*> _extensions);

    QueueFamilyIndices 
        vku_FindQueueFamilies
        (VkPhysicalDevice _device, VkSurfaceKHR _surface);

    SwapchainSupportDetails 
        vku_QuerySwapChainSupport
        (VkPhysicalDevice _device, VkSurfaceKHR _surface);

    VkSurfaceFormatKHR 
        vku_ChooseSwapSurfaceFormat
        (const _vec<VkSurfaceFormatKHR>& _availableFormats);

    VkPresentModeKHR 
        vku_ChooseSwapPresentMode
        (const _vec<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D 
        vku_ChooseSwapExtent
        (const VkSurfaceCapabilitiesKHR& _capabilities, GLFWwindow* _window);
}
}
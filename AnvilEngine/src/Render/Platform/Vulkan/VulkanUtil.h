#pragma once

#include "Util/UMacros.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>

// validates that a VK API call returns VK_SUCCESS. Otherwise, throws an error
#define VK_CHECK_RESULT(f, msg) {                                                                \
    VkResult res = (f);                                                                          \
    if (res != VK_SUCCESS) {                                                                     \
        throw std::runtime_error(                                                                \
            std::string("Vulkan Error: ") + std::to_string(res) +                                \
            "\nFile: " + __FILE__ +                                                              \
            "\nFunction: " + __FUNCTION__ +                                                      \
            "\nLine: " + std::to_string(__LINE__) +                                              \
            std::string(" : ") + std::string(msg));                                              \
    }                                                                                            \
    else                                                                                         \
    {                                                                                            \
        std::cout << "\033[38;5;40m[" << __FUNCTION__ << "] >> VK CHECK RESULT PASSED\033[0m\n";                     \
    }                                                                                            \
}                                                                                                \

namespace anv
{

    struct QueueFamilyIndices;
    struct SwapChainSupportDetails;

namespace vk_util {
        struct VKCDebugInfo
        {
            VkDebugUtilsMessengerEXT debugMessenger;

            // Validation Layers
            inline static _vec(const char*) Layers = {
                "VK_LAYER_KHRONOS_validation",
            };

            inline static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
                VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData) {

                std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
                return VK_FALSE;
            }

            VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator);

            void DestroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator);
        };

        // Retrieves the list of required Vulkan instance extensions.
        _vec(const char*) VKUtil_GetRequiredExtensions();

        // Check if this device supports validation layers
        bool VKUtil_CheckValidationSupport();

        bool VkUtil_CheckDeviceExtensionSupport(VkPhysicalDevice _device, const _vec(const char*) _extensions);

        VkPhysicalDevice VKUtil_FindSuitableDevice(_vec(VkPhysicalDevice) _devices, 
            VkSurfaceKHR _surface, const _vec(const char*) _extensions);

        QueueFamilyIndices VKUtil_FindQueueFamilies(VkPhysicalDevice _device, VkSurfaceKHR _surface);

        SwapChainSupportDetails VkUtil_QuerySwapChainSupport(VkPhysicalDevice _device, VkSurfaceKHR _surface);

        VkSurfaceFormatKHR VKUtil_ChooseSwapSurfaceFormat(const _vec(VkSurfaceFormatKHR) _availableFormats);
}
}
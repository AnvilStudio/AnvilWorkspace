#pragma once

#include "Render/Context/Context.h"
#include "Util/UMacros.h"

#include "VulkanUtil.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <optional>



namespace anv {

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();;
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanContext
        : public Context
    {
    public:
        VulkanContext(Window* _win);
        ~VulkanContext();

    public:
        void CreateBuffer() override {};

    private:
        void vkc_instance(); // instance creation
        void vkc_surface(Window* _win); // rendering surface
        void vkc_physical(); // select gpu
        void vkc_logical(); // create logical device

    private:

        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
        VkDevice m_Device;
        VkPhysicalDevice m_PhysicalDevice;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
        
        const _vec(const char*) m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    #ifdef DEBUG
        vk_util::VKCDebugInfo m_DebugInfo;
    #endif // DEBUG
    };
}
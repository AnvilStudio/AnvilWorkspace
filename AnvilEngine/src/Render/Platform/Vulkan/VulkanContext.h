#pragma once

#include "Render/Context.h"
#include "Util/UMacros.h"
#include "VulkanUtil.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanSwapChain.h"
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <optional>



namespace anv {

    class VulkanSwapChain;

    class VulkanContext
        : public Context
    {
    public:
        VulkanContext(Window* _win);
        ~VulkanContext();

    public:
        VkInstance        GetInstance()       { return m_Instance;        } 
        VkSurfaceKHR      GetSurface()        { return m_Surface;         }
        VkDevice          GetDevice()         { return m_Device;          }
        VkPhysicalDevice  GetPhysicalDevice() { return m_PhysicalDevice;  }
        VkQueue           GetGraphicsQueue()  { return m_GraphicsQueue;   }
        VkQueue           GetPresentQueue()   { return m_PresentQueue;    }
        GLFWwindow*       GetWinHandle()      { return m_WinHandle;       }
        VulkanSwapchain&  GetSwapchain()      { return m_Swapchain;       }

    private:
        void vkc_instance(); // instance creation
        void vkc_surface (); // rendering surface
        void vkc_physical(); // select gpu
        void vkc_logical (); // create logical device

    private:
        // switched to a raw vulkan swapchain
        VulkanSwapchain  m_Swapchain;
        GLFWwindow*      m_WinHandle;
        VkInstance       m_Instance;
        VkSurfaceKHR     m_Surface;
        VkDevice         m_Device;
        VkPhysicalDevice m_PhysicalDevice;
        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;
        
        const _vec<const char*> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    #ifdef DEBUG
        vk_util::VKDebugInfo m_DebugInfo;
    #endif // DEBUG
    };
}
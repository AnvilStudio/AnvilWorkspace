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
        : public Context, public std::enable_shared_from_this<VulkanContext>
    {
    public:
        VulkanContext(Window* _win);
        ~VulkanContext() override;

        void CreateSwapchain() override;

    public:
        VkInstance           GetInstance()            { return m_Instance;            } 
        VkSurfaceKHR      GetSurface()              { return m_Surface;             }
        VkDevice              GetDevice()               { return m_Device;              }
        VkPhysicalDevice  GetPhysicalDevice()   { return m_PhysicalDevice;  }
        VkQueue              GetGraphicsQueue()  { return m_GraphicsQueue; }
        VkQueue              GetPresentQueue()    { return m_PresentQueue;   }
        VkCommandPool  GetCommandPool()   { return m_CmdPool;          }
        GLFWwindow*       GetWinHandle()         { return m_WinHandle;       }
        void                      IdleDevice();

        // Returned as a VkSwapchain because
        // We're in the abstract class.
        Ref<VulkanSwapchain>    
        GetSwapchain() { return m_Swapchain; }

    private:
        void vkc_instance(); // instance creation
        void vkc_surface (); // rendering surface
        void vkc_physical(); // select gpu
        void vkc_logical (); // create logical device
        void vkc_cmd_pool();

    private:
        VkDevice                 m_Device;
        VkInstance              m_Instance;
        VkSurfaceKHR         m_Surface;
        VkCommandPool    m_CmdPool;
        VkPhysicalDevice    m_PhysicalDevice;
        VkQueue                m_GraphicsQueue;
        VkQueue                m_PresentQueue;
        
        _vec<const char*> m_DeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

    #ifdef DEBUG
        vk_util::VKDebugInfo m_DebugInfo;
    #endif // DEBUG
    };
}
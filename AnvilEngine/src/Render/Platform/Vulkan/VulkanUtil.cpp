
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
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        _vec<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    #ifdef DEBUG
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    #ifdef __APPLE__
        // Required on macOS with MoltenVK
        extensions.push_back("VK_KHR_portability_enumeration");
        extensions.push_back("VK_MVK_macos_surface");
    #endif

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

        return true;
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

            // Geometry shader is a bonus!
            if (!deviceFeatures.geometryShader) {
                score += 1000; 
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

        ANV_ASSERT(_device != VK_NULL_HANDLE, "vku_FindQueueFamilies: _device is null!");
        
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

    void vku_ToVulkanAttachmentDescription(RenderPass::Attachment* _att, VkAttachmentDescription* _desc)
    {
        switch (_att->type)
        {
            // Color attachment
        case RenderPass::Attachment::Type::ATT_TY_COLOR:
            // load ops
            switch (_att->loadOp)
            {
            case RenderPass::Attachment::LoadOp::LOAD_OP_CLEAR:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;
            case RenderPass::Attachment::LoadOp::LOAD_OP_LOAD:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                break;
            case RenderPass::Attachment::LoadOp::LOAD_OP_UNDEF:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
            case RenderPass::Attachment::LoadOp::LOAD_OP_MAX_ENUM:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                ANV_LOG_WARN("Vk Render Pass \"%s\" attachment %i has an unusable load op (MAX_ENUM)\nSetting to clear op", _att->d_rp_name, _att->d_index)
                    break;
            default:
                ANV_LOG_ERROR("Vk Render Pass \"%s\" attachment %i has an unknown load op", _att->d_rp_name, _att->d_index)
                    break;
            }

            // store ops
            switch (_att->storeOp)
            {
            case RenderPass::Attachment::StoreOp::STORE_OP_STORE:
                _desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                break;
            case RenderPass::Attachment::StoreOp::STORE_OP_UNDEF:
                _desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
            case RenderPass::Attachment::StoreOp::STORE_OP_MAX_ENUM:
                _desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                ANV_LOG_WARN("Vk Render Pass \"%s\" attachment %i has an unusable store op (MAX_ENUM)\nSetting to undef op", _att->d_rp_name, _att->d_index)
                    break;
            default:
                ANV_LOG_ERROR("Vk Render Pass \"%s\" attachment %i has an unknown store op", _att->d_rp_name, _att->d_index)
                    break;
            }
            break;

            // Depth Attachment
        case RenderPass::Attachment::Type::ATT_TY_DEPTH:
            // load ops
            switch (_att->loadOp)
            {
            case RenderPass::Attachment::LoadOp::LOAD_OP_CLEAR:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                break;
            case RenderPass::Attachment::LoadOp::LOAD_OP_LOAD:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                break;
            case RenderPass::Attachment::LoadOp::LOAD_OP_UNDEF:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                break;
            case RenderPass::Attachment::LoadOp::LOAD_OP_MAX_ENUM:
                _desc->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                ANV_LOG_WARN("Vk Render Pass \"%s\" attachment %i has an unusable load op (MAX_ENUM)\nSetting to clear op", _att->d_rp_name, _att->d_index)
                    break;
            default:
                ANV_LOG_ERROR("Vk Render Pass \"%s\" attachment %i has an unknown load op", _att->d_rp_name, _att->d_index)
                    break;
            }

            // store ops 
            switch (_att->storeOp)
            {
            case RenderPass::Attachment::StoreOp::STORE_OP_STORE:
                _desc->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                break;
            case RenderPass::Attachment::StoreOp::STORE_OP_UNDEF:
                _desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                break;
            case RenderPass::Attachment::StoreOp::STORE_OP_MAX_ENUM:
                _desc->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                ANV_LOG_WARN("Vk Render Pass \"%s\" attachment %i has an unusable store op (MAX_ENUM)\nSetting to undef op", _att->d_rp_name, _att->d_index)
                    break;
            default:
                ANV_LOG_ERROR("Vk Render Pass \"%s\" attachment %i has an unknown store op", _att->d_rp_name, _att->d_index)
                    break;
            }
            break;
        }
    }

    void vku_ToRenderPassLayout(RenderPass::Attachment* _att, VkAttachmentDescription* _desc)
    {
        switch (_att->beginLayout)
        {
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_COLOR_ATT:
            _desc->initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_PRES:
            _desc->initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_MEMCPY_DST:
            _desc->initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_UNDEF:
            _desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_MAX_ENUM:
            ANV_LOG_WARN("Vk render pass \"%s\" %i has an unusable begining layout (MAX_ENUM)\nSetting to undef", _att->d_rp_name, _att->d_index)
                _desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        default:
            ANV_LOG_ERROR("Vk render pass \"%s\" %i has an unknown begining layout", _att->d_rp_name, _att->d_index);
            break;
        }

        switch (_att->endLayout)
        {
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_COLOR_ATT:
            _desc->finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_PRES:
            _desc->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_MEMCPY_DST:
            _desc->finalLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_UNDEF:
            _desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        case RenderPass::Attachment::ImgLayout::IMG_LAYOUT_MAX_ENUM:
            ANV_LOG_WARN("Vk render pass \"%s\" %i has an unusable final layout (MAX_ENUM)\nSetting to undef", _att->d_rp_name, _att->d_index)
                _desc->finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        default:
            ANV_LOG_ERROR("Vk render pass \"%s\" %i has an unknown final layout", _att->d_rp_name, _att->d_index);
            break;
        }
    }
    VkFormat vku_ToImageFormat(Image2D::Format _fmt)
    {
        switch (_fmt)
        {
        case anv::Image2D::Format::UNDEF:
            ANV_LOG_ERROR("Image fmt set to UNDEF... setting as R8G8B8A8_UNorm")
            return VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case anv::Image2D::Format::R8G8B8A8_UNorm:
            return VK_FORMAT_R8G8B8A8_UNORM;
            break;
        case anv::Image2D::Format::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
            break;
        case anv::Image2D::Format::B8G8R8A8_UNorm:
            return VK_FORMAT_B8G8R8A8_UNORM;
            break;
        case anv::Image2D::Format::B8G8R8A8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
            break;
        case anv::Image2D::Format::D24_UNorm_S8_UInt:
            return VK_FORMAT_D24_UNORM_S8_UINT;
            break;
        case anv::Image2D::Format::D32_SFloat:
            return VK_FORMAT_D32_SFLOAT;
            break;
        case anv::Image2D::Format::D32_SFloat_S8_UInt:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
            break;
        case anv::Image2D::Format::R16G16B16A16_SFloat:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
            break;
        case anv::Image2D::Format::R32G32B32A32_SFloat:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
            break;
        default:
            ANV_LOG_ERROR("Image fmt set to ???... setting as R8G8B8A8_UNorm")
            return VK_FORMAT_R8G8B8A8_UNORM;
            break;
        }
    }
    Image2D::Format vku_ToEngineImgFormat(VkFormat _fmt)
    {
        switch (_fmt)
        {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return anv::Image2D::Format::R8G8B8A8_UNorm;
        case VK_FORMAT_R8G8B8A8_SRGB:
            return anv::Image2D::Format::R8G8B8A8_SRGB;
        case VK_FORMAT_B8G8R8A8_UNORM:
            return anv::Image2D::Format::B8G8R8A8_UNorm;
        case VK_FORMAT_B8G8R8A8_SRGB:
            return anv::Image2D::Format::B8G8R8A8_SRGB;
        case VK_FORMAT_D24_UNORM_S8_UINT:
            return anv::Image2D::Format::D24_UNorm_S8_UInt;
        case VK_FORMAT_D32_SFLOAT:
            return anv::Image2D::Format::D32_SFloat;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return anv::Image2D::Format::D32_SFloat_S8_UInt;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return anv::Image2D::Format::R16G16B16A16_SFloat;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return anv::Image2D::Format::R32G32B32A32_SFloat;
        default:
            ANV_LOG_WARN("Unknown VkFormat provided... defaulting to anv::Image2D::Format::UNDEF")
                return anv::Image2D::Format::UNDEF;
        }
    }
}
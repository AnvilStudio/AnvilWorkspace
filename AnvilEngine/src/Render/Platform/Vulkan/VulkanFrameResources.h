#pragma once
#include <vulkan/vulkan.h>
#include "Render/CommandBuffer.h" // your base
#include "Core/Reference.h"

namespace anv
{
    struct VulkanFrameSync
    {
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
        VkFence     inFlightFence = VK_NULL_HANDLE;
    };

    // Holds the Command buffer and sync resources
    struct VulkanFrameResources
    {
        Ref<CommandBuffer> cmd;   
        VulkanFrameSync    sync;
    };
}


#pragma once
#include "../../Framebuffer.h"
#include "VulkanRenderPass.h"
#include "VulkanContext.h"

#include <vulkan/vulkan.h>

namespace anv
{
    class VulkanFrameBuffer :
        public Framebuffer
    {
    public:
        VulkanFrameBuffer(_shared<Context> _ctx, Ref<ImageView> _imgv, Ref<RenderPass> _rp, uint32_t _width, uint32_t _height);
        ~VulkanFrameBuffer();

        VkFramebuffer Get() { return m_FrameBuffer; }
    private:
        void create_frame_buffer();

    private:
        VkFramebuffer  m_FrameBuffer = VK_NULL_HANDLE;
    };
}
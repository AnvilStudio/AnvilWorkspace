#include "Image.h"
#include "RenderAPI.h"
#include "Context.h"
#include "Swapchain.h"
#include "Platform/Vulkan/VulkanImage.h"

namespace anv
{
    //Ref<ImageView> ImageView::Create(_shared<Context> _ctx, Ref<Swapchain> _sc)
    //{
    //    switch (RenderAPI::GetAPI())
    //    {
    //    //case GraphicsAPI::VK: return Ref<VulkanImageView>::Create(_ctx, _sc);
    //    //default:
    //    //    ANV_LOG_ERROR("GraphicsAPI not supported, using vulkan");
    //    //    return Ref<VulkanImageView>::Create(_ctx, _sc);
    //    }
    //}

    Ref<Image2D> anv::Image2D::Create(_shared<Context> _ctx, Format _fmt, uint32_t _width, uint32_t _height)
    {
        switch (RenderAPI::GetAPI())
        {
        case GraphicsAPI::VK: return Ref<VulkanImage2D>::Create(_ctx, _fmt, _width, _height);
        }
    }

    Image2D::Image2D(_shared<Context> _ctx, uint32_t _width, uint32_t _height)
        : m_Context(_ctx), m_Width(_width), m_Height(_height)
    {
    }
}
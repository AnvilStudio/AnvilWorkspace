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

    Ref<Image2D> anv::Image2D::Create(_shared<Context> _ctx, Format _fmt)
    {
        switch (RenderAPI::GetAPI())
        {
        case GraphicsAPI::VK: return Ref<VulkanImage2D>::Create(_ctx, _fmt);
        }
    }

    Image2D::Image2D(_shared<Context> _ctx)
        : m_Context(_ctx)
    {
    }
}
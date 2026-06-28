#pragma once
#include "../../RenderTarget.h"
#include <vulkan/vulkan.h>
namespace anv
{
    class VulkanSwapchainRenderTarget : public RenderTarget
    {
    public:
        VulkanSwapchainRenderTarget(_shared<Context> ctx)
            : RenderTarget(
                ctx,
                RenderTargetType::RENDER_TARGET_TYPE_IMAGE,
                ctx->GetSwapchain()->GetExtent().width,
                ctx->GetSwapchain()->GetExtent().height
            )
        {
            create_renderpass();
            create_framebuffers();
        }

        uint32_t AcquireNextImage(VkSemaphore imageAvailable, bool& recreate);
        void Present(VkQueue presentQueue, VkSemaphore renderFinished, bool& recreate);

       RenderTargetType GetType() override { return m_Type; }
       uint32_t GetWidth() override { return m_Width; }
       uint32_t GetHeight() override { return m_Height; }
       void Resize(uint32_t _width, uint32_t _height) override;
       Ref<Image2D> GetImage() { return nullptr; }
       Ref<RenderPass> GetRenderPass() override { return m_Renderpass; }
       void* GetImGuiTextureID() override { return nullptr; }
        Ref<Framebuffer> GetFrameBuffer() override
        {
            return m_Framebuffers[m_ImageIndex];
        } 

       void Begin(Ref<CommandBuffer> _cmd) override;
       void End(Ref<CommandBuffer> _cmd) override;

    private:
        uint32_t m_ImageIndex = 0;
        _vec<Ref<Framebuffer>> m_Framebuffers;

        void create_framebuffers();
        void create_renderpass();
    };
}

#pragma once
#include "../../RenderTarget.h"
#include <vulkan/vulkan.h>

namespace anv
{
    class VulkanRenderTarget :
        public RenderTarget
    {
    public:
        VulkanRenderTarget(_shared<Context> _ctx, RenderTargetType _type, uint32_t _width, uint32_t _height);
        ~VulkanRenderTarget();

        virtual RenderTargetType GetType() override;
        virtual uint32_t GetWidth() override;
        virtual uint32_t GetHeight() override;

        virtual void Resize(uint32_t _width, uint32_t _height) override;

        virtual Ref<Image2D> GetImage() override;
        virtual Ref<RenderPass> GetRenderPass() override;
        virtual Ref<Framebuffer> GetFrameBuffer() override;

        virtual void Begin(Ref<CommandBuffer> _cmd) override;
        virtual void End(Ref<CommandBuffer> _cmd) override;

        virtual void* GetImGuiTextureID();

    private:
        void create_framebuffer();
        void create_sampler();
        void create_renderpass();
        void create_image();

        VkDescriptorSet m_ImGuiDescriptor = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;
        Ref<ImageView> m_ImageView;
    };
}

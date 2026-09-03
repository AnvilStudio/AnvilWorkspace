#pragma once

#include <Render/RenderTarget.h>

namespace anv
{
    class MetalContext;

    class MetalRenderTarget : public RenderTarget
    {
    public:
        MetalRenderTarget(
            _shared<Context> _context,
            RenderTargetType _type,
            uint32_t _width,
            uint32_t _height);

        ~MetalRenderTarget() override;

        RenderTargetType GetType() override;
        uint32_t GetWidth() override;
        uint32_t GetHeight() override;

        void Resize(uint32_t _width, uint32_t _height) override;

        Ref<Image2D> GetImage() override;
        Ref<RenderPass> GetRenderPass() override;
        Ref<Framebuffer> GetFrameBuffer() override;

        void Begin(Ref<CommandBuffer> _commandBuffer) override;
        void End(Ref<CommandBuffer> _commandBuffer) override;

        void* GetImGuiTextureID() override;

        void* GetTexture() const;

    private:
        void create_texture();
        void destroy_texture();

    private:
        _shared<MetalContext> m_MetalContext = nullptr;
        void* m_Texture = nullptr;
    };
}
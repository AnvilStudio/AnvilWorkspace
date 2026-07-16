#include "MtlRenderTarget.h"
#include "MtlContext.h"

#import <Metal/Metal.h>

namespace anv
{
    MetalRenderTarget::MetalRenderTarget(
        _shared<Context> _context,
        RenderTargetType _type,
        uint32_t _width,
        uint32_t _height)
        : RenderTarget(_context, _type, _width, _height)
    {
        m_MetalContext =
            std::dynamic_pointer_cast<MetalContext>(_context);

        ANV_ASSERT(
            m_MetalContext,
            "MetalRenderTarget received a non-Metal context!");

        create_texture();
    }

    MetalRenderTarget::~MetalRenderTarget()
    {
        destroy_texture();
    }

    RenderTargetType MetalRenderTarget::GetType()
    {
        return m_Type;
    }

    uint32_t MetalRenderTarget::GetWidth()
    {
        return m_Width;
    }

    uint32_t MetalRenderTarget::GetHeight()
    {
        return m_Height;
    }

    void MetalRenderTarget::Resize(
        uint32_t _width,
        uint32_t _height)
    {
        if (_width == 0 || _height == 0)
            return;

        if (_width == m_Width && _height == m_Height)
            return;

        m_Width = _width;
        m_Height = _height;

        create_texture();
    }

    Ref<Image2D> MetalRenderTarget::GetImage()
    {
        return m_Image;
    }

    Ref<RenderPass> MetalRenderTarget::GetRenderPass()
    {
        return m_Renderpass;
    }

    Ref<Framebuffer> MetalRenderTarget::GetFrameBuffer()
    {
        return m_Framebuffer;
    }

    void MetalRenderTarget::Begin(
        Ref<CommandBuffer> _commandBuffer)
    {
        (void)_commandBuffer;
    }

    void MetalRenderTarget::End(
        Ref<CommandBuffer> _commandBuffer)
    {
        (void)_commandBuffer;
    }

    void* MetalRenderTarget::GetImGuiTextureID()
    {
        return m_Texture;
    }

    void* MetalRenderTarget::GetTexture() const
    {
        return m_Texture;
    }

    void MetalRenderTarget::create_texture()
    {
        destroy_texture();

        if (!m_MetalContext || m_Width == 0 || m_Height == 0)
            return;

        id<MTLDevice> device =
            (__bridge id<MTLDevice>)
                m_MetalContext->GetDevice();

        MTLTextureDescriptor* descriptor =
            [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:
                    MTLPixelFormatBGRA8Unorm
                width:m_Width
                height:m_Height
                mipmapped:NO];

        descriptor.textureType = MTLTextureType2D;
        descriptor.storageMode = MTLStorageModePrivate;
        descriptor.usage =
            MTLTextureUsageRenderTarget |
            MTLTextureUsageShaderRead;

        id<MTLTexture> texture =
            [device newTextureWithDescriptor:descriptor];

        ANV_ASSERT(
            texture,
            "Failed to create Metal render-target texture!");

        texture.label = @"Anvil Offscreen Render Target";

        m_Texture = (__bridge_retained void*)texture;
    }

    void MetalRenderTarget::destroy_texture()
    {
        if (!m_Texture)
            return;

        CFBridgingRelease(m_Texture);
        m_Texture = nullptr;
    }
}
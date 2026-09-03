#include "MtlTexture.h"

#include <Core/App.h>
#include "MtlContext.h"

#import <Metal/Metal.h>

namespace anv
{
    MetalTexture::MetalTexture(const std::filesystem::path& path)
        : Texture(path)
    {
        Upload();
    }

    MetalTexture::MetalTexture(Deserialized& deserialized)
        : Texture(deserialized)
    {
        Upload();
    }

    MetalTexture::~MetalTexture()
    {
        Destroy();
    }

    bool MetalTexture::IsGPUReady() const
    {
        return m_Texture != nullptr;
    }

    void* MetalTexture::GetNativeHandle() const
    {
        return m_Texture;
    }

    void MetalTexture::Upload()
    {
        Destroy();

        if (!m_Data || m_Width <= 0 || m_Height <= 0)
            return;

        auto context = std::dynamic_pointer_cast<MetalContext>(
            App::GetInstance()->GetMainWindow()->GetContext());

        if (!context)
        {
            ANV_LOG_ERROR("MetalTexture could not acquire a MetalContext");
            return;
        }

        id<MTLDevice> device =
            (__bridge id<MTLDevice>)context->GetDevice();

        MTLTextureDescriptor* descriptor =
            [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                               width:static_cast<NSUInteger>(m_Width)
                                                              height:static_cast<NSUInteger>(m_Height)
                                                           mipmapped:NO];

        descriptor.usage = MTLTextureUsageShaderRead;
        descriptor.storageMode = MTLStorageModeShared;

        id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
        if (!texture)
        {
            ANV_LOG_ERROR("Failed to create Metal texture: %s", m_Name.c_str());
            return;
        }

        const MTLRegion region = MTLRegionMake2D(
            0,
            0,
            static_cast<NSUInteger>(m_Width),
            static_cast<NSUInteger>(m_Height));

        [texture replaceRegion:region
                   mipmapLevel:0
                     withBytes:m_Data
                   bytesPerRow:static_cast<NSUInteger>(m_Width * 4)];

        texture.label = [NSString stringWithUTF8String:m_Name.c_str()];
        m_Texture = (__bridge_retained void*)texture;
    }

    void MetalTexture::Destroy()
    {
        if (m_Texture)
        {
            CFBridgingRelease(m_Texture);
            m_Texture = nullptr;
        }
    }
}

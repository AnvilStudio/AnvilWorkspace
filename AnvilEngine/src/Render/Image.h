#pragma once

#include "../Core/Reference.h"
#include "../Util/UMacros.h"

namespace anv
{
    class Context;
    class Swapchain;


    // IMG VIEW =======
    class ImageView :
        public RefCounter
    {
    public:
        //ANV_NO_DSCRD
        //static Ref<ImageView> Create(_shared<Context> _ctx, Ref<Swapchain> _sc);

        virtual void OnDestroy() = 0;
        virtual ~ImageView() = default;

        //virtual void OnDestroy() = 0;
    };


    // IMAGE  =======
    class Image2D :
        public RefCounter
    {
    public:

        enum class Usage : uint32_t
        {
            NONE = 0,

            SAMPLED,
            COLOR_ATTACHMENT,
            DEPTH_STENCIL,

            TRANSFER_SRC,
            TRANSFER_DST,

            STORAGE,

            PRESENT
        };

        enum class Format
        {
            UNDEF,                   // Default or uninitialized format
            R8G8B8A8_UNorm,          // 8-bit RGBA format (normalized)
            R8G8B8A8_SRGB,           // 8-bit RGBA format (sRGB color space)
            B8G8R8A8_UNorm,          // 8-bit BGRA format (normalized)
            B8G8R8A8_SRGB,           // 8-bit BGRA format (sRGB color space)
            D24_UNorm_S8_UInt,       // Depth 24-bit + Stencil 8-bit
            D32_SFloat,              // 32-bit float depth format
            D32_SFloat_S8_UInt,      // 32-bit float depth + 8-bit stencil
            R16G16B16A16_SFloat,     // 16-bit float per channel (RGBA)
            R32G32B32A32_SFloat,     // 32-bit float per channel (RGBA)
        };

        ANV_NO_DSCRD
        static Ref<Image2D> Create(_shared<Context> _ctx, Format _fmt, uint32_t _width, uint32_t _height);

        Image2D(_shared<Context> _ctx, uint32_t _width, uint32_t _height);
        Image2D() = default;

        ANV_NO_DSCRD
        virtual Ref<ImageView> MakeImageView() = 0;

    protected:
        _shared<Context> m_Context;
        uint32_t m_Width;
        uint32_t m_Height;
    };
}

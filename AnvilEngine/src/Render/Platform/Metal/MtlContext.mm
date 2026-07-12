#include "MtlContext.h"
#include "Core/Window.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace anv
{
    MetalContext::MetalContext(Window* win)
        : Context(win)
    {
        m_Device = MTLCreateSystemDefaultDevice();
        m_CommandQueue = [m_Device newCommandQueue];
    }

    void MetalContext::WaitIdle()
    {
        // TODO
    }

    void MetalContext::CreateSwapchain()
    {
        // TODO
    }
}
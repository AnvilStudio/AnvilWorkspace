#include "MtlContext.h"
#include "Core/Window.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


namespace anv
{
    MetalContext::MetalContext(Window* win)
        : Context(win)
    {
        create_device();
        create_command_queue();
        create_layer();
    }

    MetalContext::~MetalContext()
    {
        m_Layer = nil;
        m_CommandQueue = nil;
        m_Device = nil;
    }

    GLFWwindow* MetalContext::GetWindowHandle() const
    {
        return m_WinHandle;
    }

    void MetalContext::create_device()
    {
        m_Device = MTLCreateSystemDefaultDevice();

        if (!m_Device)
        {
            ANV_LOG_FATAL("Failed to create Metal device!");
        }
    }

    void MetalContext::create_command_queue()
    {
        m_CommandQueue = [m_Device newCommandQueue];

        if (!m_CommandQueue)
        {
            ANV_LOG_FATAL("Failed to create Metal command queue!");
        }
    }

    void MetalContext::create_layer()
    {
        NSWindow* window = glfwGetCocoaWindow(m_WinHandle);

        if (!window)
        {
            ANV_LOG_FATAL("Failed to retrieve Cocoa window from GLFW!");
            return;
        }

        NSView* view = window.contentView;
        view.wantsLayer = YES;

        m_Layer = [CAMetalLayer layer];
        m_Layer.device = m_Device;
        m_Layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
        m_Layer.framebufferOnly = YES;
        m_Layer.contentsScale = window.backingScaleFactor;
        m_Layer.frame = view.bounds;
        m_Layer.autoresizingMask =
            kCALayerWidthSizable | kCALayerHeightSizable;
        #if TARGET_OS_OSX
        m_Layer.displaySyncEnabled = YES;
        #endif
        view.layer = m_Layer;

        CreateSwapchain();
    }

    void MetalContext::WaitIdle()
    {
        id<MTLCommandBuffer> commandBuffer =
            [m_CommandQueue commandBuffer];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
    }

    void MetalContext::CreateSwapchain()
    {
        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(m_WinHandle, &width, &height);

        m_Layer.drawableSize = CGSizeMake(width, height);
    }

    void* MetalContext::GetDevice() const
    {
        return (__bridge void*)m_Device;
    }

    void* MetalContext::GetCommandQueue() const
    {
        return (__bridge void*)m_CommandQueue;
    }

    void* MetalContext::GetLayer() const
    {
        return (__bridge void*)m_Layer;
    }
}
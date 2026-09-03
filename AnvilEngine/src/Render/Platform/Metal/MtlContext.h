#pragma once

#include <Render/Context.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#else
class MTLDevice;
class MTLCommandQueue;
class CAMetalLayer;
#endif

struct GLFWwindow;

namespace anv
{
    class MetalContext : public Context
    {
    public:
        explicit MetalContext(Window* window);
        ~MetalContext() override;

        void WaitIdle() override;
        void CreateSwapchain() override;

        void* GetDevice() const;
        void* GetCommandQueue() const;
        void* GetLayer() const;

        GLFWwindow* GetWindowHandle() const;

    private:
        void create_device();
        void create_command_queue();
        void create_layer();

    private:
#ifdef __OBJC__
        id<MTLDevice>        m_Device = nil;
        id<MTLCommandQueue>  m_CommandQueue = nil;
        CAMetalLayer*        m_Layer = nil;
#else
        void* m_Device = nullptr;
        void* m_CommandQueue = nullptr;
        void* m_Layer = nullptr;
#endif
    };
}
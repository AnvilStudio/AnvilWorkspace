#pragma once
#include "../Util/UMacros.h"
#include "Swapchain.h"

struct GLFWwindow;

namespace anv {

    class RenderAPI;
    class Window;
    struct Render2DCreateInfo;

    class Context
    {
    public:
        static _shared<Context> Create(Window* _win);
        virtual ~Context() = default;

        _shared<RenderAPI> InitAPI(Render2DCreateInfo _info);

        // probably needs to be in VkRenderAPI instead of the base class
        // MTL doesnt really have a controllable swapchain 
        virtual void CreateSwapchain() = 0;
        // this one too ...
        inline Ref<Swapchain> GetSwapchain() { return m_Swapchain; }
        
        inline _shared<RenderAPI> GetAPI() { return m_API; }

        virtual void WaitIdle() = 0;

        template<typename T>
        inline T* GetAs() { return static_cast<T*>(this); }

        Context(Window* _win);

    protected:
        GLFWwindow* m_WinHandle;
        Ref<Swapchain>     m_Swapchain;
        _shared<RenderAPI> m_API;
    };
}

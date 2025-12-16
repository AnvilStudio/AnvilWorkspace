///////////////////////////////////////////////////////////////////
//                                                               //
// Renderer.h - The base class prototype for 2D and 3D rendering //
//                                                               //
// This file serves as a prototype for the Renderer2D and        //
// Renderer3D classes. It defines core rendering functions and   //
// interfaces that can be extended for different rendering       //
// backends (e.g., Vulkan, OpenGL, DirectX, Metal, etc.).        //
//                                                               //
// TODO (Alba): Implement Metal renderer.                        //
///////////////////////////////////////////////////////////////////

#pragma once

#include "../Util/UMacros.h"
#include "../Core/QueueChain.h"
#include "Context.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
#include "Camera.h"

#include <string>
#include "Framebuffer.h"

namespace anv {

    struct Color {
        float r, g, b, a;
    };

    enum class RenderingPipeline {
        FWD, // Forward+
        DFR, // Differed
        TWD  // 2D
    };

    struct Render3DCreateInfo
    {
        _shared<Window> pTarget = nullptr;

        // Graphics API
        GraphicsAPI api = GraphicsAPI::VK;

        // Resolution and V-Sync
        int width = 1920;
        int height = 1080;
        bool vsyncEnabled = true;
        int targetFrameRate = 60;

        // Anti-Aliasing
        int msaaSamples = 4;

        // Swapchain
        int swapchainImageCount = 3;

        // Depth and Stencil
        bool enableDepthBuffer = true;
        int depthBits = 24;
        int stencilBits = 8;

        // Clear color
        Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Off-Screen Rendering
        bool offscreenRendering = false;

        // Debug
        bool enableDebugMode = false;

        // Rendering Pipeline
        RenderingPipeline pipeline = RenderingPipeline::FWD;

        // Asset paths
        std::string shaderPath = "assets/shaders/";
        std::string texturePath = "assets/textures/";

        // Shadows
        int shadowMapResolution = 1024;

        // Camera / Projection settings
        float fov = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    };

    struct Render2DCreateInfo
    {
        // Presentation
        _shared<Window> pTarget = nullptr;

        // Graphics API
        GraphicsAPI api = GraphicsAPI::VK;

        // Batching settings
        int MAX_QUADS_PER_BATCH = 9;

        // Resolution and V-Sync
        int width = 1920;
        int height = 1080;
        bool vsyncEnabled = true;
        // 0 - Unlimited F.R.
        int targetFrameRate = 60;

        // Anti-Aliasing
        bool enableMSAA = false;
        int msaaSamples = 4;

        // Swapchain
        int swapchainImageCount = 3;

        // Depth and Stencil
        // Not Set Up!
        bool enableDepthBuffer = false;
        int depthBits = 24;
        int stencilBits = 8;

        // Clear color
        Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Off-Screen Rendering
        bool offscreenRendering = false;

        // Debug
        bool enableDebugMode = false;

        // Rendering Pipeline
        RenderingPipeline pipeline = RenderingPipeline::TWD;

        // Asset paths
        std::string assetPath   = "Assets";

        // AnvEngine specific assets
        std::string shaderPath  = "Assets/com.anvstu.engine/shaders";
        std::string texturePath = "Assets/com.anvstu.engine/textures";

        // Shadows
        //int shadowMapResolution = 1024;

        // Camera / Projection settings
        float fov = 45.0f;
    };

    class Renderer2D
    {
    public:

        // API //
        static void Init(Render2DCreateInfo _info);
        static void Shutdown();
        static void BeginFrame();
        static void EndFrame();

        static Render2DCreateInfo GetSettings() { return m_RenderCreateInfo; }

        static void SetCamera(_shared<Camera2D> _main);
        //static void CmdDrawQuad(VertexBuffer& _vb, IndexBuffer& _ib);
        //static void CmdDrawQuadWithMaterial(Material& _mat, VertexBuffer& _vb, IndexBuffer& _ib);
        //static void CmdDrawQuadWithTexture(Texture& _text, VertexBuffer& _vb, IndexBuffer& _ib);
        //static void CmdSubmit();

    private:
        static void create_frame_buffers();
        static void set_pipeline();
        static void set_shaders();

    private:
        inline static _shared<Camera2D>         m_MainCamera       = nullptr;
        inline static _shared<RenderAPI>        m_RenderAPI        = nullptr;
        inline static Ref<GraphicsPipeline>     m_Pipeline         = nullptr;
        inline static Ref<RenderPass>           m_RenderPass       = nullptr;
        inline static _vec<Ref<Framebuffer>>    m_FrameBuffers     {};
        inline static Render2DCreateInfo        m_RenderCreateInfo {};
        
        //inline static QueueChain* m_RenderCmdChain = nullptr;
    };
}

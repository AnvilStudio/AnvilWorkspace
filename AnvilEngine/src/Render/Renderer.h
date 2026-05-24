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
#include "Context.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
#include "Camera.h"
#include "RenderAPI.h"
#include <string>
#include "Framebuffer.h"

namespace anv {

    struct Color {
        float r, g, b, a;

        //glm::vec4 operator=(const Color&& _col)
        //{
        //    return glm::vec4(_col.r, _col.g, _col.b, _col.a);
        //}
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
        bool enableDepthBuffer = false;
        int depthBits = 24;
        int stencilBits = 8;

        // Clear color
        Color clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        // Off-Screen Rendering
        bool offscreenRendering = false;

        // Debug
        bool enableDebugMode = false;

        // Asset paths
        std::string assetPath   = "Assets";

        // Shadows
        int shadowMapResolution = 1024;

        // Camera / Projection settings
        float fov = 45.0f;
    };

    class Renderer2D
    {
    public:

        // API //
        static void Init(Render2DCreateInfo _info);
        static void Shutdown();
        static void DrawFrame();

        static Render2DCreateInfo GetSettings() { return m_RenderCreateInfo; }

        static void SetCamera(_shared<Camera2D> _main);

        static void BeginScene();
        static void EndScene();

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, glm::vec4 color);
        
        //static void CmdDrawQuadWithMaterial(Material& _mat, VertexBuffer& _vb, IndexBuffer& _ib);
        //static void CmdDrawQuadWithTexture(Texture& _text, VertexBuffer& _vb, IndexBuffer& _ib);
        //static void CmdSubmit();


    protected:
        //inline static _shared<Camera2D>        m_MainCamera     = nullptr;
        inline static _shared<RenderAPI>        m_RenderAPI        = nullptr;
        inline static Render2DCreateInfo         m_RenderCreateInfo {};
        
    };
}

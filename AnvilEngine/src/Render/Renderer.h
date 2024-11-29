#pragma once

#include "Context.h"
#pragma once
#include "../Util/UMacros.h"
#include "Swapchain.h"
#include "Context.h"

#include <string>

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
        bool enableDebugMode = true;

        // Rendering Pipeline
        RenderingPipeline pipeline = RenderingPipeline::TWD;

        // Asset paths
        std::string shaderPath = "shaders/";
        std::string texturePath = "textures/";

        // Shadows
        //int shadowMapResolution = 1024;

        // Camera / Projection settings
        float fov = 45.0f;
    };

    class Renderer2D
    {
    public:
        Renderer2D(Render2DCreateInfo _info);
        ~Renderer2D();

        // API //
        void BeginFrame();
        void EndFrame();

        //void DrawQuad(VertexBuffer& _vb, IndexBuffer& _ib);
        //void DrawQuadWithMaterial(Material& _mat, VertexBuffer& _vb, IndexBuffer& _ib);
        //void DrawQuadWithTexture(Texture& _text, VertexBuffer& _vb, IndexBuffer& _ib);

    private:
        _shared<RenderAPI>  m_RenderAPI;
        Render2DCreateInfo& m_RenderInfo;
        // RenderQueue*  m_CmdQueue
        // RenderThread* m_RenderThread
    };
}

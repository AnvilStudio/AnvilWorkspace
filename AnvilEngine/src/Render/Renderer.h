#pragma once

#include "Context/Context.h"
#include "../Util/UMacros.h"
#include "Swapchain.h"

#include <string>

namespace anv {

    struct Color {
        float r, g, b, a;
    };

    enum class RenderingPipeline {
        Forward,
        Deferred
    };

    enum class GraphicsAPI {
        Vulkan,
        OpenGL,
        DirectX,
        Metal
    };

    struct RenderCreateInfo
    {
        _shared(Window) pTarget = nullptr;

        // Graphics API
        GraphicsAPI api = GraphicsAPI::Vulkan;

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
        RenderingPipeline pipeline = RenderingPipeline::Forward;

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

    class Renderer2D
    {
    public:
        Renderer2D(RenderCreateInfo _info);
        ~Renderer2D();

        // API //
        void BeginFrame();
        void EndFrame();

        void DrawQuad();
        void DrawQuadWithMaterial();
        void DrawQuadWithTexture();

    private:
        RenderCreateInfo& m_RenderInfo;

        // RenderQueue*  m_CmdQueue
        // RenderThread* m_RenderThread
    };
}

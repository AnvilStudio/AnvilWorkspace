#pragma once

#include "../Util/UMacros.h"
#include "Context.h"
#include "Swapchain.h"
#include "GraphicsPipeline.h"
#include "Camera.h"
#include "RenderAPI.h"
#include "Framebuffer.h"
#include "RenderStats.h"
#include <string>

namespace anv
{
    class Texture;

    struct Color
    {
        float r, g, b, a;
    };

    enum class RenderingPipeline
    {
        FWD,
        DFR,
        TWD
    };

    struct Render3DCreateInfo
    {
        _shared<Window> pTarget = nullptr;
        GraphicsAPI api = GraphicsAPI::VK;
        int width = 1920;
        int height = 1080;
        bool vsyncEnabled = true;
        int targetFrameRate = 60;
        int msaaSamples = 4;
        int swapchainImageCount = 3;
        bool enableDepthBuffer = true;
        int depthBits = 24;
        int stencilBits = 8;
        Color clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        bool offscreenRendering = false;
        bool enableDebugMode = false;
        RenderingPipeline pipeline = RenderingPipeline::FWD;
        std::string shaderPath = "assets/shaders/";
        std::string texturePath = "assets/textures/";
        int shadowMapResolution = 1024;
        float fov = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
    };

    struct Render2DCreateInfo
    {
        _shared<Window> pTarget = nullptr;
        GraphicsAPI api = GraphicsAPI::VK;
        int MAX_QUADS_PER_BATCH = 9;
        int width = 1920;
        int height = 1080;
        bool vsyncEnabled = true;
        int targetFrameRate = 60;
        bool enableMSAA = false;
        int msaaSamples = 4;
        int swapchainImageCount = 3;
        bool enableDepthBuffer = false;
        int depthBits = 24;
        int stencilBits = 8;
        Color clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
        bool offscreenRendering = false;
        bool enableDebugMode = false;
        std::string assetPath = "Assets";
        std::string imguiIniPath = "imgui.ini";
        int shadowMapResolution = 1024;
        float fov = 45.0f;
    };

    class Renderer2D
    {
    public:
        static void Init(Render2DCreateInfo info);
        static void Shutdown();
        static void DrawFrame();

        static Render2DCreateInfo GetSettings() { return m_RenderCreateInfo; }
        static RendererStats GetStats() { return m_RenderAPI->GetStats(); }

        static void SetCamera(_shared<Camera2D> mainCamera);
        static void BeginScene();
        static void DrawScene(Ref<RenderTarget> renderTarget, _shared<Camera2D> camera);
        static void EndScene();

        static void DrawQuad(
            const glm::vec2& position,
            float rotation,
            const glm::vec2& size,
            glm::vec4 color,
            Ref<Texture> texture = nullptr,
            int layer = 0);

        static void WaitIdle();

    protected:
        inline static _shared<RenderAPI> m_RenderAPI = nullptr;
        inline static Render2DCreateInfo m_RenderCreateInfo{};
    };
}

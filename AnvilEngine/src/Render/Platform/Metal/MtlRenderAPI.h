#pragma once

#include <Render/RenderAPI.h>
#include "MtlContext.h"

namespace anv
{
    class MetalRenderAPI : public RenderAPI
    {
    public:
        explicit MetalRenderAPI(Render2DCreateInfo info);
        ~MetalRenderAPI() override;

        RendererStats GetStats() override;

        void DrawFrame() override;
        void OnShutdown() override;

        void BeginScene() override;
        void BeginScene(Ref<RenderTarget> renderTarget) override;
        void DrawScene(Ref<RenderTarget> renderTarget) override;

        void DrawQuad(
            const glm::vec2& position,
            float rotation,
            const glm::vec2& size,
            glm::vec4 color,
            int layer
        ) override;

        void EndScene() override;

        void SetMainCamera(_shared<Camera2D> camera) override;

    private:
        void create_triangle_pipeline();

        void initialize_imgui();
        void shutdown_imgui();
    private:
        _shared<MetalContext> m_MetalContext = nullptr;

        // Stored opaquely because this header is included by normal C++.
        void* m_TrianglePipeline = nullptr;

        RendererStats m_Stats{};

        void* m_CurrentDrawable = nullptr;
        void* m_CurrentCommandBuffer = nullptr;
        void* m_CurrentRenderPass = nullptr;
        void* m_CurrentEncoder = nullptr;
    };
}
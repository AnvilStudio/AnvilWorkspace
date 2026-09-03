#pragma once

#include <Render/RenderAPI.h>
#include <Render/RenderData.h>
#include "MtlContext.h"
#include "Layer/ImGuiLayer.h"

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
        void DrawScene(Ref<RenderTarget> renderTarget, _shared<Camera2D> camera) override;

        void DrawQuad(
            const glm::vec2& position,
            float rotation,
            const glm::vec2& size,
            glm::vec4 color,
            Ref<Texture> texture,
            int layer) override;

        void EndScene() override;
        void SetMainCamera(_shared<Camera2D> camera) override;

    private:
        void create_sprite_pipeline();
        void encode_quads(void* encoder, _shared<Camera2D> camera);

    private:
        _shared<MetalContext> m_MetalContext = nullptr;
        void* m_SpritePipeline = nullptr;
        void* m_Sampler = nullptr;
        _vec<QuadSubmission> m_QuadQueue{};
        RendererStats m_Stats{};
        std::unique_ptr<ImGuiLayer> m_ImGuiLayer;
        bool m_HasShutdown = false;
        void* m_CurrentDrawable = nullptr;
        void* m_CurrentCommandBuffer = nullptr;
        void* m_CurrentRenderPass = nullptr;
        void* m_CurrentEncoder = nullptr;
    };
}

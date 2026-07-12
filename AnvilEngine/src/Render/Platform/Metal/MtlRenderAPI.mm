#include "MtlRenderAPI.h"
#include <Render/Renderer.h>
#include <Core/Window.h>

namespace anv
{
    MetalRenderAPI::MetalRenderAPI(Render2DCreateInfo info)
    {
        m_Context = info.pTarget->GetContext();
    }

    MetalRenderAPI::~MetalRenderAPI() = default;

    RendererStats MetalRenderAPI::GetStats()
    {
        return m_Stats;
    }

    void MetalRenderAPI::DrawFrame()
    {
        if (m_CurrentTarget)
        {
            DrawScene(m_CurrentTarget);
        }
        EndScene();
    }

    void MetalRenderAPI::OnShutdown()
    {
        //m_CurrentTarget.reset();
        //m_Camera.reset();
        //m_MetalContext.reset();
        //m_Context.reset();
    }

    void MetalRenderAPI::BeginScene()
    {
        // Setup default scene state if needed.
    }

    void MetalRenderAPI::BeginScene(Ref<RenderTarget> _renderTarget)
    {
        m_CurrentTarget = _renderTarget;
    }

    void MetalRenderAPI::DrawScene(Ref<RenderTarget> _renderTarget)
    {
        (void)_renderTarget;
        // Metal draw submission should be implemented here.
    }

    void MetalRenderAPI::DrawQuad(const glm::vec2& position, float rotation, const glm::vec2& size, glm::vec4 color, int layer)
    {
        (void)position;
        (void)rotation;
        (void)size;
        (void)color;
        (void)layer;
        // Record quad rendering commands for Metal.
    }

    void MetalRenderAPI::EndScene()
    {
        // Submit Metal command buffer / present swapchain.
    }

    void MetalRenderAPI::SetMainCamera(_shared<Camera2D> camera)
    {
        m_Camera = camera;
    }
}
#include "Renderer.h"
#include "Shader.h"
#include "GraphicsPipeline.h"
#include <Core/App.h>

namespace anv
{
    void Renderer2D::Init(Render2DCreateInfo info)
    {
        ANV_PROFILE_SCOPE();
        m_RenderCreateInfo = info;
        m_RenderAPI = info.pTarget->GetContext()->InitAPI(info);
    }

    void Renderer2D::Shutdown()
    {
        m_RenderAPI->OnShutdown();
    }

    void Renderer2D::SetCamera(_shared<Camera2D> mainCamera)
    {
        m_RenderAPI->SetMainCamera(mainCamera);
    }

    void Renderer2D::BeginScene()
    {
        m_RenderAPI->BeginScene();
    }

    void Renderer2D::DrawScene(Ref<RenderTarget> renderTarget)
    {
        m_RenderAPI->DrawScene(renderTarget);
    }

    void Renderer2D::EndScene()
    {
        m_RenderAPI->EndScene();
    }

    void Renderer2D::DrawQuad(
        const glm::vec2& position,
        float rotation,
        const glm::vec2& size,
        glm::vec4 color,
        Ref<Texture> texture,
        int layer)
    {
        m_RenderAPI->DrawQuad(position, rotation, size, color, texture, layer);
    }

    void Renderer2D::DrawFrame()
    {
        m_RenderAPI->DrawFrame();
    }

    void Renderer2D::WaitIdle()
    {
        App::GetInstance()->GetMainWindow()->GetContext()->WaitIdle();
    }
}

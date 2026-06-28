#include "Viewport.h"

Viewport::Viewport()
{
    m_ViewportTarget = anv::RenderTarget::Create(
        anv::App::GetInstance()->GetMainWindow()->GetContext(),
        anv::RenderTargetType::RENDER_TARGET_TYPE_IMAGE,
        300,
        175
    );

    m_Camera = anv::App::GetInstance()->GetSceneManager()->GetActive()->GetMainCamera();
}

void Viewport::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    static ImVec2 last_size{};

    bool validSize = viewportSize.x > 0 && viewportSize.y > 0;
    bool sizeChanged =
        validSize &&
        (last_size.x != viewportSize.x || last_size.y != viewportSize.y);

    if (sizeChanged)
    {
        // IMPORTANT:
        // Do not immediately destroy GPU resources if render thread may be using them.
        anv::Renderer2D::WaitIdle(); // temporary safe fix
        m_ViewportTarget->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

        last_size = viewportSize;
        m_Camera->SetAspectRatio((viewportSize.x / viewportSize.y));
    }

    if (validSize)
    {
        anv::Renderer2D::DrawScene(m_ViewportTarget);

        ImGui::Image(
            m_ViewportTarget->GetImGuiTextureID(),
            viewportSize
        );
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

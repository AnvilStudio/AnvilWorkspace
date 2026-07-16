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

    bool validSize = viewportSize.x > 0 && viewportSize.y > 0;

    ImVec2 framebufferScale =
        ImGui::GetIO().DisplayFramebufferScale;

    uint32_t targetWidth = validSize
        ? static_cast<uint32_t>(
            viewportSize.x * framebufferScale.x)
        : 0;

    uint32_t targetHeight = validSize
        ? static_cast<uint32_t>(
            viewportSize.y * framebufferScale.y)
        : 0;

    bool sizeChanged =
        validSize &&
        targetWidth > 0 &&
        targetHeight > 0 &&
        (m_LastTargetWidth != targetWidth ||
         m_LastTargetHeight != targetHeight);

    if (sizeChanged && m_ViewportTarget)
    {
        // IMPORTANT:
        // Do not immediately destroy GPU resources if render thread may be using them.
        anv::Renderer2D::WaitIdle(); // temporary safe fix
        m_ViewportTarget->Resize(targetWidth, targetHeight);

        m_LastTargetWidth = targetWidth;
        m_LastTargetHeight = targetHeight;
        m_Camera->SetAspectRatio((viewportSize.x / viewportSize.y));
    }

    if (validSize && m_ViewportTarget)
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
#include "GameViewport.h"
#include "../EditorLayer.h"
#include <algorithm>
#include <cctype>

GameViewport::GameViewport()

{
    m_GameViewportTarget = anv::RenderTarget::Create(
        anv::App::GetInstance()->GetMainWindow()->GetContext(),
        anv::RenderTargetType::RENDER_TARGET_TYPE_IMAGE,
        300,
        175);
}

void GameViewport::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game Viewport");

    if (EditorLayer::GetSceneState() == EditorLayer::SceneState::Play)
    {
        ImGui::SetWindowFocus("Game Viewport");
    }

    auto sceneManager = anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
    auto sceneCamera = scene->GetMainCamera();

    if (scene)
    {

        const ImVec2 currentSize = ImGui::GetContentRegionAvail();
        if (sceneCamera && currentSize.x > 0.0f && currentSize.y > 0.0f)
            sceneCamera->SetAspectRatio(currentSize.x / currentSize.y);
    }

    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    const bool validSize = viewportSize.x > 0 && viewportSize.y > 0;
    ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;

    uint32_t targetWidth = validSize
                               ? static_cast<uint32_t>(viewportSize.x * framebufferScale.x)
                               : 0;
    uint32_t targetHeight = validSize
                                ? static_cast<uint32_t>(viewportSize.y * framebufferScale.y)
                                : 0;

    const bool sizeChanged =
        validSize && targetWidth > 0 && targetHeight > 0 &&
        (m_LastTargetWidth != targetWidth || m_LastTargetHeight != targetHeight);

    if (sizeChanged && m_GameViewportTarget)
    {
        anv::Renderer2D::WaitIdle();
        m_GameViewportTarget->Resize(targetWidth, targetHeight);
        m_LastTargetWidth = targetWidth;
        m_LastTargetHeight = targetHeight;

        if (sceneCamera)
            sceneCamera->SetAspectRatio(viewportSize.x / viewportSize.y);
    }

    if (validSize && m_GameViewportTarget)
    {
        anv::Renderer2D::DrawScene(m_GameViewportTarget, sceneCamera);
        ImGui::Image(m_GameViewportTarget->GetImGuiTextureID(), viewportSize);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

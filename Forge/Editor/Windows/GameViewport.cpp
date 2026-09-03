#include "GameViewport.h"
#include "../EditorLayer.h"

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

    auto sceneManager = anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
    auto sceneCamera = scene ? scene->GetActiveCamera() : nullptr;

    if (EditorLayer::GetSceneState() == EditorLayer::SceneState::Play)
    {
        if (!m_OnPlayFocused)
        {
            m_OnPlayFocused = true;
            ImGui::SetWindowFocus();
        }
        m_InputEnabled = true;
    }
    else
    {
        m_OnPlayFocused = false;
        m_InputEnabled = false;
    }

    const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    const bool validSize = viewportSize.x > 0.0f && viewportSize.y > 0.0f;
    const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;

    const uint32_t targetWidth = validSize
                                     ? static_cast<uint32_t>(viewportSize.x * framebufferScale.x)
                                     : 0;

    const uint32_t targetHeight = validSize
                                      ? static_cast<uint32_t>(viewportSize.y * framebufferScale.y)
                                      : 0;

    const bool sizeChanged =
        validSize && targetWidth > 0 && targetHeight > 0 &&
        (m_LastTargetWidth != targetWidth ||
         m_LastTargetHeight != targetHeight);

    if (sizeChanged && m_GameViewportTarget)
    {
        anv::Renderer2D::WaitIdle();
        m_GameViewportTarget->Resize(targetWidth, targetHeight);
        m_LastTargetWidth = targetWidth;
        m_LastTargetHeight = targetHeight;
    }

    if (sceneCamera && validSize)
    {
        sceneCamera->SetAspectRatio(viewportSize.x / viewportSize.y);
        sceneCamera->Update(0.0f);
    }

    if (validSize && m_GameViewportTarget && sceneCamera)
    {
        anv::Renderer2D::DrawScene(m_GameViewportTarget, sceneCamera);

#if defined(PLATFORM_APPLE_VK) || defined(PLATFORM_WIN64)
        const ImVec2 uv0(0.0f, 1.0f);
        const ImVec2 uv1(1.0f, 0.0f);
#else
        const ImVec2 uv0(1.0f, 0.0f);
        const ImVec2 uv1(0.0f, 1.0f);
#endif

        ImGui::Image(
            m_GameViewportTarget->GetImGuiTextureID(),
            viewportSize,
            uv0,
            uv1);
    }
    else if (validSize && !sceneCamera)
    {
        const char *message = "No active Camera2D in scene";
        const ImVec2 textSize = ImGui::CalcTextSize(message);
        ImGui::SetCursorPos(ImVec2(
            (viewportSize.x - textSize.x) * 0.5f,
            (viewportSize.y - textSize.y) * 0.5f));
        ImGui::TextDisabled("%s", message);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

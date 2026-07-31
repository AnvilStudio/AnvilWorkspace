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

void GameViewport::synchronize_scene_runtime()
{
    auto sceneManager = anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
    if (!scene)
        return;

    const bool isPlaying =
        EditorLayer::GetSceneState() == EditorLayer::SceneState::Play;

    if (isPlaying == m_WasPlaying)
        return;

    if (isPlaying)
    {
        scene->Save();
        scene->SetScriptExecutionEnabled(true);
        scene->StartPhysics();
        ANV_LOG_INFO("Scene runtime started.")
    }
    else
    {
        scene->SetScriptExecutionEnabled(false);
        scene->StopPhysics();
        EditorLayer::GetInstance()->ClearSelection();
        sceneManager->ReloadActive();
        ANV_LOG_INFO("Scene runtime stopped and edit scene restored.")
    }

    m_WasPlaying = isPlaying;
}

void GameViewport::Draw()
{
    synchronize_scene_runtime();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Game Viewport");

    auto sceneManager = anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
    auto sceneCamera = scene ? scene->GetMainCamera() : nullptr;

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

    if (scene)
    {
        //scene->SetCameraInputEnabled(m_InputEnabled);

        const ImVec2 currentSize = ImGui::GetContentRegionAvail();
        if (sceneCamera && currentSize.x > 0.0f && currentSize.y > 0.0f)
            sceneCamera->SetAspectRatio(currentSize.x / currentSize.y);
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

        if (sceneCamera)
            sceneCamera->SetAspectRatio(viewportSize.x / viewportSize.y);
    }

    if (validSize && m_GameViewportTarget && sceneCamera)
    {
        anv::Renderer2D::DrawScene(m_GameViewportTarget, sceneCamera);
        ImGui::Image(m_GameViewportTarget->GetImGuiTextureID(), viewportSize);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

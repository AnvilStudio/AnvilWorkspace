#include "Viewport.h"
#include "../EditorLayer.h"
#include <algorithm>
#include <cctype>

namespace
{
    bool IsTextureFile(const std::filesystem::path& _path)
    {
        std::string extension = _path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char _character)
            {
                return static_cast<char>(std::tolower(_character));
            });

        return extension == ".png" ||
               extension == ".jpg" ||
               extension == ".jpeg" ||
               extension == ".bmp" ||
               extension == ".tga";
    }
}

Viewport::Viewport()
    : m_Controller(anv::App::GetInstance()->GetInputSystem(),
     m_EditorCamera)
{
    m_ViewportTarget = anv::RenderTarget::Create(
        anv::App::GetInstance()->GetMainWindow()->GetContext(),
        anv::RenderTargetType::RENDER_TARGET_TYPE_IMAGE,
        300,
        175);

    m_EditorCamera = std::make_shared<anv::Camera2D>();

    anv::Renderer2D::SetCamera(m_EditorCamera);
}

void Viewport::Draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport");

    auto sceneManager = anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;

    if (scene)
    {

        const ImVec2 currentSize = ImGui::GetContentRegionAvail();
        if (m_EditorCamera && currentSize.x > 0.0f && currentSize.y > 0.0f)
            m_EditorCamera->SetAspectRatio(currentSize.x / currentSize.y);
        

        if (EditorLayer::GetSceneState() == EditorLayer::SceneState::Edit)
        {
            m_Controller.SetInputEnabled(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
        } else {
            m_Controller.SetInputEnabled(false);
        }
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

    if (sizeChanged && m_ViewportTarget)
    {
        anv::Renderer2D::WaitIdle();
        m_ViewportTarget->Resize(targetWidth, targetHeight);
        m_LastTargetWidth = targetWidth;
        m_LastTargetHeight = targetHeight;

        if (m_EditorCamera)
            m_EditorCamera->SetAspectRatio(viewportSize.x / viewportSize.y);
    }

    if (validSize && m_ViewportTarget)
    {
        anv::Renderer2D::DrawScene(m_ViewportTarget, m_EditorCamera);
        ImGui::Image(m_ViewportTarget->GetImGuiTextureID(), viewportSize);

        // Scene Drag/Drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
            {
                const auto* pathData = static_cast<const char*>(payload->Data);
                const std::filesystem::path assetPath(pathData ? pathData : "");

                if (scene && IsTextureFile(assetPath))
                {
                    auto assetManager = anv::App::GetInstance()->GetAssetManager();
                    auto texture = assetManager->GetOrCreateTexture(assetPath);

                    if (texture && texture->IsGPUReady())
                    {
                        const entt::entity entity =
                            scene->CreateEntity(assetPath.stem().string());
                        auto& sprite =
                            scene->AddComponent<anv::Component::SpriteRenderer>(entity);
                        sprite.texture = texture->GetAssetID();
                        scene->Save();
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

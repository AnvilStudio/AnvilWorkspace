#include "Viewport.h"
#include "../EditorLayer.h"

#include <algorithm>
#include <cctype>
#include <limits>

#include <glm/gtc/type_ptr.hpp>

namespace
{
    bool IsTextureFile(const std::filesystem::path &_path)
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
{
    m_ViewportTarget = anv::RenderTarget::Create(
        anv::App::GetInstance()->GetMainWindow()->GetContext(),
        anv::RenderTargetType::RENDER_TARGET_TYPE_IMAGE,
        300,
        175);

    m_EditorCamera = std::make_shared<anv::Camera2D>();

    m_Controller.SetCamera(m_EditorCamera);
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

        m_Controller.SetInputEnabled(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
        m_Controller.Update(anv::Time::DeltaTime());

        update_operation(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
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

        ImGuizmo::SetOrthographic(true);
        ImGuizmo::SetDrawlist();

        set_gizmo_bounds();
        draw_gizmo();

        if (scene)
            pick_entity(scene);

        // Scene Drag/Drop
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload =
                    ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
            {
                const auto *pathData = static_cast<const char *>(payload->Data);
                const std::filesystem::path assetPath(pathData ? pathData : "");

                if (scene && IsTextureFile(assetPath))
                {
                    auto assetManager = anv::App::GetInstance()->GetAssetManager();
                    auto texture = assetManager->GetOrCreateTexture(assetPath);

                    if (texture && texture->IsGPUReady())
                    {
                        const entt::entity entity =
                            scene->CreateEntity(assetPath.stem().string());
                        auto &sprite =
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

void Viewport::pick_entity(
    const anv::Ref<anv::Scene>& scene
)
{
    if (!scene || !m_EditorCamera)
        return;

    if (EditorLayer::GetSceneState() !=
        EditorLayer::SceneState::Edit)
    {
        return;
    }

    if (!m_ViewportHovered ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
        ImGuizmo::IsOver() ||
        ImGuizmo::IsUsing())
    {
        return;
    }

    const glm::vec2 mousePosition{
        ImGui::GetMousePos().x,
        ImGui::GetMousePos().y
    };

    const glm::vec2 localMouse =
        mousePosition - m_ViewportBounds[0];

    if (localMouse.x < 0.0f ||
        localMouse.y < 0.0f ||
        localMouse.x > m_ViewportSize.x ||
        localMouse.y > m_ViewportSize.y ||
        m_ViewportSize.x <= 0.0f ||
        m_ViewportSize.y <= 0.0f)
    {
        return;
    }

    const glm::vec2 normalized{
        localMouse.x / m_ViewportSize.x,
        localMouse.y / m_ViewportSize.y
    };

    const glm::vec4 clipPosition{
        normalized.x * 2.0f - 1.0f,
        1.0f - normalized.y * 2.0f,
        0.0f,
        1.0f
    };

    const glm::mat4 inverseViewProjection =
        glm::inverse(
            m_EditorCamera->GetProjection() *
            m_EditorCamera->GetView()
        );

    glm::vec4 worldPosition =
        inverseViewProjection * clipPosition;

    if (worldPosition.w != 0.0f)
        worldPosition /= worldPosition.w;

    entt::entity pickedEntity = entt::null;
    int highestDrawLayer = std::numeric_limits<int>::min();

    auto view = scene->Registry().view<
        anv::Component::Transform2d,
        anv::Component::SpriteRenderer>();

    for (const auto entity : view)
    {
        const auto& transform =
            view.get<anv::Component::Transform2d>(entity);

        const auto& sprite =
            view.get<anv::Component::SpriteRenderer>(entity);

        const glm::mat4 inverseTransform =
            glm::inverse(transform.GetTransform());

        const glm::vec4 localPosition =
            inverseTransform * glm::vec4(
                worldPosition.x,
                worldPosition.y,
                0.0f,
                1.0f
            );

        const bool insideQuad =
            localPosition.x >= -0.5f &&
            localPosition.x <= 0.5f &&
            localPosition.y >= -0.5f &&
            localPosition.y <= 0.5f;

        if (!insideQuad)
            continue;

        if (sprite.drawLayer >= highestDrawLayer)
        {
            highestDrawLayer = sprite.drawLayer;
            pickedEntity = entity;
        }
    }

    if (pickedEntity == entt::null)
    {
        EditorLayer::GetInstance()->ClearSelection();
        return;
    }

    EditorLayer::GetInstance()->SetSelectedEntity(
        pickedEntity
    );
}

void Viewport::update_operation(bool update)
{
    if (!update)
        return;

    auto is = anv::App::GetInstance()->GetInputSystem();

    if (is->IsKeyPressed(ANV_KEY_1))
    {
        m_Operation = ImGuizmo::OPERATION::TRANSLATE;
    }
    if (is->IsKeyPressed(ANV_KEY_2))
    {
        m_Operation = ImGuizmo::OPERATION::ROTATE;
    }
    if (is->IsKeyPressed(ANV_KEY_3))
    {
        m_Operation = ImGuizmo::OPERATION::SCALE;
    }
}

void Viewport::set_gizmo_bounds()
{
    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsWindowHovered();

    const ImVec2 viewportMinRegion =
        ImGui::GetWindowContentRegionMin();

    const ImVec2 viewportMaxRegion =
        ImGui::GetWindowContentRegionMax();

    const ImVec2 viewportOffset =
        ImGui::GetWindowPos();

    m_ViewportBounds[0] = {
        viewportMinRegion.x + viewportOffset.x,
        viewportMinRegion.y + viewportOffset.y};

    m_ViewportBounds[1] = {
        viewportMaxRegion.x + viewportOffset.x,
        viewportMaxRegion.y + viewportOffset.y};

    m_ViewportSize = {
        m_ViewportBounds[1].x - m_ViewportBounds[0].x,
        m_ViewportBounds[1].y - m_ViewportBounds[0].y};

    ImGuizmo::SetRect(
        m_ViewportBounds[0].x,
        m_ViewportBounds[0].y,
        m_ViewportSize.x,
        m_ViewportSize.y);
}

void Viewport::draw_gizmo()
{
    auto sceneManager = anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;

    if (!scene)
        return;

    auto entity = EditorLayer::GetInstance()->GetSelectedEntity();
    if (entity == entt::null || !scene->Registry().valid(entity))
        return;

    if (!scene->HasComponent<anv::Component::Transform2d>(entity))
        return;

    auto &transform = scene->GetComponent<anv::Component::Transform2d>(entity);

    glm::mat4 transformMatrix =
        transform.GetTransform();

    const glm::mat4 &view =
        m_EditorCamera->GetView();

    const glm::mat4 &projection =
        m_EditorCamera->GetProjection();

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        m_Operation,
        ImGuizmo::LOCAL,
        glm::value_ptr(transformMatrix));

    if (ImGuizmo::IsUsing())
    {
        transform.SetFromMatrix(transformMatrix);
    }
}

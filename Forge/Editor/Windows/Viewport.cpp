#include "Viewport.h"
#include "../EditorLayer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
    bool IsTextureFile(const std::filesystem::path &path)
    {
        std::string extension = path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
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

        bool camInputEn =
            (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
             ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows));

        m_Controller.SetInputEnabled(camInputEn);

        m_Controller.Update(anv::Time::DeltaTime());

        update_operation(
            ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));
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
        ImGui::Image(
            m_ViewportTarget->GetImGuiTextureID(),
            viewportSize,
            ImVec2(0.0f, 1.0f),
            ImVec2(1.0f, 0.0f));
            
        set_gizmo_bounds();
        
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *payload =
                    ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
            {
                const auto *pathData =
                    static_cast<const char *>(payload->Data);
                const std::filesystem::path assetPath(
                    pathData ? pathData : "");

                if (scene && IsTextureFile(assetPath))
                {
                    auto assetManager =
                        anv::App::GetInstance()->GetAssetManager();
                    auto texture =
                        assetManager->GetOrCreateTexture(assetPath);

                    if (texture && texture->IsGPUReady())
                    {
                        const entt::entity entity =
                            scene->CreateEntity(assetPath.stem().string());

                        auto &sprite =
                            scene->AddComponent<
                                anv::Component::SpriteRenderer>(entity);

                        sprite.texture = texture->GetAssetID();
                        scene->Save();
                    }
                }
            }

            ImGui::EndDragDropTarget();
        }
    }
    
    draw_selection_overlay(scene);

    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist();
    draw_gizmo();

    if (scene)
        pick_entity(scene);

    ImGui::End();
    ImGui::PopStyleVar();
}

bool Viewport::world_to_viewport(
    const glm::vec3 &worldPosition,
    ImVec2 &screenPosition) const
{
    if (!m_EditorCamera ||
        m_ViewportSize.x <= 0.0f ||
        m_ViewportSize.y <= 0.0f)
    {
        return false;
    }

    const glm::mat4 viewProjection =
        m_EditorCamera->GetProjection() *
        m_EditorCamera->GetView();

    const glm::vec4 clipPosition =
        viewProjection * glm::vec4(worldPosition, 1.0f);

    if (std::abs(clipPosition.w) <= 0.00001f)
        return false;

    const glm::vec3 ndc =
        glm::vec3(clipPosition) / clipPosition.w;

    screenPosition.x =
        m_ViewportBounds[0].x +
        (ndc.x * 0.5f + 0.5f) * m_ViewportSize.x;

    screenPosition.y =
        m_ViewportBounds[0].y +
        (1.0f - (ndc.y * 0.5f + 0.5f)) * m_ViewportSize.y;

    return true;
}

void Viewport::draw_selection_overlay(
    const anv::Ref<anv::Scene> &scene)
{
    if (!scene || !m_EditorCamera)
        return;

    const entt::entity entity =
        EditorLayer::GetInstance()->GetSelectedEntity();

    if (entity == entt::null ||
        !scene->Registry().valid(entity) ||
        !scene->HasComponent<anv::Component::Transform2d>(entity))
    {
        return;
    }

    const auto &transform =
        scene->GetComponent<anv::Component::Transform2d>(entity);

    ImDrawList *drawList = ImGui::GetWindowDrawList();
    if (!drawList)
        return;

    const ImU32 outlineColor = IM_COL32(255, 214, 10, 255);
    const ImU32 activeCameraColor = IM_COL32(80, 220, 120, 255);
    const ImU32 shadowColor = IM_COL32(0, 0, 0, 190);
    const ImU32 iconFillColor = IM_COL32(45, 45, 50, 235);
    constexpr float outlineThickness = 2.0f;
    constexpr float shadowThickness = 4.0f;
    constexpr float pivotRadius = 5.0f;

    ImVec2 pivot{};
    if (!world_to_viewport(
            glm::vec3(transform.position, 0.0f),
            pivot))
    {
        return;
    }

    drawList->PushClipRect(
        ImVec2(m_ViewportBounds[0].x, m_ViewportBounds[0].y),
        ImVec2(m_ViewportBounds[1].x, m_ViewportBounds[1].y),
        true);

    if (scene->HasComponent<anv::Component::Camera2D>(entity))
    {
        auto &cameraComponent =
            scene->GetComponent<anv::Component::Camera2D>(entity);

        if (!cameraComponent.camera)
            cameraComponent.camera = std::make_shared<anv::Camera2D>();

        const float halfHeight =
            std::max(0.1f, cameraComponent.camera->GetZoom());
        const float halfWidth =
            halfHeight * std::max(0.01f, cameraComponent.camera->GetAspectRatio());

        const glm::mat4 cameraTransform =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(transform.position, 0.0f)) *
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(transform.rotation),
                glm::vec3(0.0f, 0.0f, 1.0f));

        const std::array<glm::vec4, 4> localFrustumCorners = {
            glm::vec4{-halfWidth, -halfHeight, 0.0f, 1.0f},
            glm::vec4{halfWidth, -halfHeight, 0.0f, 1.0f},
            glm::vec4{halfWidth, halfHeight, 0.0f, 1.0f},
            glm::vec4{-halfWidth, halfHeight, 0.0f, 1.0f}};

        std::array<ImVec2, 4> frustumCorners{};
        bool validFrustum = true;

        for (std::size_t index = 0;
             index < localFrustumCorners.size();
             ++index)
        {
            const glm::vec4 worldCorner =
                cameraTransform * localFrustumCorners[index];

            if (!world_to_viewport(
                    glm::vec3(worldCorner),
                    frustumCorners[index]))
            {
                validFrustum = false;
                break;
            }
        }

        const ImU32 cameraColor = cameraComponent.isActive
                                      ? activeCameraColor
                                      : outlineColor;

        if (validFrustum)
        {
            drawList->AddPolyline(
                frustumCorners.data(),
                static_cast<int>(frustumCorners.size()),
                shadowColor,
                ImDrawFlags_Closed,
                shadowThickness);

            drawList->AddPolyline(
                frustumCorners.data(),
                static_cast<int>(frustumCorners.size()),
                cameraColor,
                ImDrawFlags_Closed,
                outlineThickness);

            const ImVec2 topMid{
                (frustumCorners[2].x + frustumCorners[3].x) * 0.5f,
                (frustumCorners[2].y + frustumCorners[3].y) * 0.5f};

            drawList->AddLine(
                pivot,
                topMid,
                cameraColor,
                1.0f);
        }

        constexpr float iconHalfWidth = 10.0f;
        constexpr float iconHalfHeight = 7.0f;
        constexpr float lensLength = 7.0f;

        const ImVec2 bodyMin{
            pivot.x - iconHalfWidth,
            pivot.y - iconHalfHeight};
        const ImVec2 bodyMax{
            pivot.x + iconHalfWidth,
            pivot.y + iconHalfHeight};

        drawList->AddRectFilled(
            bodyMin,
            bodyMax,
            iconFillColor,
            2.0f);
        drawList->AddRect(
            bodyMin,
            bodyMax,
            cameraColor,
            2.0f,
            0,
            outlineThickness);

        const std::array<ImVec2, 3> lens = {
            ImVec2{bodyMax.x, pivot.y - 5.0f},
            ImVec2{bodyMax.x + lensLength, pivot.y - 9.0f},
            ImVec2{bodyMax.x + lensLength, pivot.y + 9.0f}};

        drawList->AddTriangleFilled(
            lens[0],
            lens[1],
            lens[2],
            iconFillColor);
        drawList->AddPolyline(
            lens.data(),
            static_cast<int>(lens.size()),
            cameraColor,
            ImDrawFlags_Closed,
            outlineThickness);

        drawList->AddCircleFilled(
            ImVec2{pivot.x - 3.0f, pivot.y},
            2.5f,
            cameraColor);
    }
    else
    {
        const glm::mat4 model = transform.GetTransform();

        constexpr std::array<glm::vec4, 4> localCorners = {
            glm::vec4{-0.5f, -0.5f, 0.0f, 1.0f},
            glm::vec4{0.5f, -0.5f, 0.0f, 1.0f},
            glm::vec4{0.5f, 0.5f, 0.0f, 1.0f},
            glm::vec4{-0.5f, 0.5f, 0.0f, 1.0f}};

        std::array<ImVec2, 4> screenCorners{};
        bool validOutline = true;

        for (std::size_t index = 0; index < localCorners.size(); ++index)
        {
            const glm::vec4 worldCorner = model * localCorners[index];
            if (!world_to_viewport(
                    glm::vec3(worldCorner),
                    screenCorners[index]))
            {
                validOutline = false;
                break;
            }
        }

        if (validOutline)
        {
            drawList->AddPolyline(
                screenCorners.data(),
                static_cast<int>(screenCorners.size()),
                shadowColor,
                ImDrawFlags_Closed,
                shadowThickness);

            drawList->AddPolyline(
                screenCorners.data(),
                static_cast<int>(screenCorners.size()),
                outlineColor,
                ImDrawFlags_Closed,
                outlineThickness);
        }
    }

    drawList->AddLine(
        ImVec2(pivot.x - pivotRadius, pivot.y),
        ImVec2(pivot.x + pivotRadius, pivot.y),
        shadowColor,
        shadowThickness);

    drawList->AddLine(
        ImVec2(pivot.x, pivot.y - pivotRadius),
        ImVec2(pivot.x, pivot.y + pivotRadius),
        shadowColor,
        shadowThickness);

    drawList->AddLine(
        ImVec2(pivot.x - pivotRadius, pivot.y),
        ImVec2(pivot.x + pivotRadius, pivot.y),
        outlineColor,
        outlineThickness);

    drawList->AddLine(
        ImVec2(pivot.x, pivot.y - pivotRadius),
        ImVec2(pivot.x, pivot.y + pivotRadius),
        outlineColor,
        outlineThickness);

    drawList->PopClipRect();
}

void Viewport::pick_entity(
    const anv::Ref<anv::Scene> &scene)
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
        ImGui::GetMousePos().y};

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
        localMouse.y / m_ViewportSize.y};

    const glm::vec4 clipPosition{
        normalized.x * 2.0f - 1.0f,
        1.0f - normalized.y * 2.0f,
        0.0f,
        1.0f};

    const glm::mat4 inverseViewProjection =
        glm::inverse(
            m_EditorCamera->GetProjection() *
            m_EditorCamera->GetView());

    glm::vec4 worldPosition =
        inverseViewProjection * clipPosition;

    if (worldPosition.w != 0.0f)
        worldPosition /= worldPosition.w;

    entt::entity pickedEntity = entt::null;
    int highestDrawLayer = std::numeric_limits<int>::min();

    auto view = scene->Registry().view<anv::Component::Transform2d, anv::Component::SpriteRenderer>();

    for (const auto entity : view)
    {
        const auto &transform =
            view.get<anv::Component::Transform2d>(entity);

        const auto &sprite =
            view.get<anv::Component::SpriteRenderer>(entity);

        const glm::mat4 inverseTransform =
            glm::inverse(transform.GetTransform());

        const glm::vec4 localPosition =
            inverseTransform * glm::vec4(
                                   worldPosition.x,
                                   worldPosition.y,
                                   0.0f,
                                   1.0f);

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

    EditorLayer::GetInstance()->SetSelectedEntity(pickedEntity);
}

void Viewport::update_operation(bool update)
{
    if (!update)
        return;

    auto inputSystem =
        anv::App::GetInstance()->GetInputSystem();

    if (inputSystem->IsKeyPressed(ANV_KEY_1))
        m_Operation = ImGuizmo::OPERATION::TRANSLATE;

    if (inputSystem->IsKeyPressed(ANV_KEY_2))
        m_Operation = ImGuizmo::OPERATION::ROTATE;

    if (inputSystem->IsKeyPressed(ANV_KEY_3))
        m_Operation = ImGuizmo::OPERATION::SCALE;
}

void Viewport::set_gizmo_bounds()
{
    m_ViewportFocused = ImGui::IsWindowFocused();
    m_ViewportHovered = ImGui::IsItemHovered();

    const ImVec2 itemMin = ImGui::GetItemRectMin();
    const ImVec2 itemMax = ImGui::GetItemRectMax();

    m_ViewportBounds[0] = {itemMin.x, itemMin.y};
    m_ViewportBounds[1] = {itemMax.x, itemMax.y};

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
    auto sceneManager =
        anv::App::GetInstance()->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;

    if (!scene || !m_EditorCamera)
        return;

    const entt::entity entity =
        EditorLayer::GetInstance()->GetSelectedEntity();

    if (entity == entt::null ||
        !scene->Registry().valid(entity) ||
        !scene->HasComponent<anv::Component::Transform2d>(entity))
    {
        return;
    }

    auto &transform =
        scene->GetComponent<anv::Component::Transform2d>(entity);

    glm::mat4 transformMatrix = transform.GetTransform();

    const glm::mat4 view = m_EditorCamera->GetView();
    const glm::mat4 projection = m_EditorCamera->GetProjection();

    //ImGuizmo::BeginFrame();

    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        m_Operation,
        ImGuizmo::LOCAL,
        glm::value_ptr(transformMatrix));

    if (ImGuizmo::IsUsing())
        transform.SetFromMatrix(transformMatrix);
}

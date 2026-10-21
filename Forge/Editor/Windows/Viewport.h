#pragma once
#include <Anvil.h>
#include "../Grid/EditorGrid.h"
#include <array>

class Viewport
{
public:
    Viewport();
    void Draw();

private:
    void draw_toolbar();
    void initialize_toolbar_icons();
    bool toolbar_icon_button(const char* id, int index, bool selected, const char* tooltip);
    void update_operation(bool update);
    void set_gizmo_bounds();
    void draw_gizmo();
    void draw_selection_overlay(const anv::Ref<anv::Scene>& scene);
    void push_billboards(const anv::Ref<anv::Scene>& scene);
    void pick_entity(const anv::Ref<anv::Scene>& scene);

    bool world_to_viewport(
        const glm::vec3& worldPosition,
        ImVec2& screenPosition
    ) const;

private:
    anv::Ref<anv::RenderTarget> m_ViewportTarget;
    anv::_shared<anv::Camera2D> m_EditorCamera;
    anv::CameraController m_Controller;
    forge::EditorGrid m_EditorGrid;
    struct ToolbarIcon
    {
        anv::Ref<anv::Texture> texture;
    };
    std::array<ToolbarIcon, 4> m_ToolbarIcons{};
    bool m_ToolbarIconsInitialized = false;
    bool m_ToolbarHovered = false;

    // Editor-only gizmo snapping; never changes runtime scene behavior.
    bool m_SnapEnabled = false;
    float m_TranslationSnap = 1.0f;
    float m_RotationSnap = 15.0f; // degrees
    float m_ScaleSnap = 0.1f;

    uint32_t m_LastTargetWidth = 0;
    uint32_t m_LastTargetHeight = 0;

    glm::vec2 m_ViewportBounds[2]{};
    glm::vec2 m_ViewportSize{0.0f};
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;

    ImGuizmo::OPERATION m_Operation = ImGuizmo::OPERATION::TRANSLATE;
};

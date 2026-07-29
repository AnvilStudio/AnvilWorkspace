#pragma once
#include <Anvil.h>

class Viewport
{
public:
    Viewport();
    void Draw();

private:
    void update_operation(bool update);
    void set_gizmo_bounds();
    void draw_gizmo();
private:
    anv::Ref<anv::RenderTarget> m_ViewportTarget;
    anv::_shared<anv::Camera2D> m_EditorCamera;
    anv::CameraController m_Controller;

    uint32_t m_LastTargetWidth = 0;
    uint32_t m_LastTargetHeight = 0;

    glm::vec2 m_ViewportBounds[2];
    glm::vec2 m_ViewportSize;
    bool m_ViewportHovered = false;
    bool m_ViewportFocused = false;

    ImGuizmo::OPERATION m_Operation = ImGuizmo::OPERATION::TRANSLATE;
};
#pragma once
#include <Anvil.h>

class Viewport
{
public:
    Viewport();
    void Draw();

private:
    anv::Ref<anv::RenderTarget> m_ViewportTarget;
    anv::_shared<anv::Camera2D> m_EditorCamera;
    anv::CameraController       m_Controller;

    uint32_t m_LastTargetWidth = 0;
    uint32_t m_LastTargetHeight = 0;
};
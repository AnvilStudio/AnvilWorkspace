#include "CameraController.h"
#include <Core/App.h>
#include <algorithm>

namespace anv
{
    CameraController::CameraController(_shared<InputSystem> _is, _shared<Camera2D> _cam)
        : m_InputSystem(_is), m_Camera(_cam)
    {
        if (m_Camera)
        {
            m_ZoomLevel = m_Camera->GetZoom();
            ANV_LOG_INFO("Camera Set");
        }
    }

    CameraController::CameraController()
    {
        m_InputSystem = App::GetInstance()->GetInputSystem();
    }

    CameraController::~CameraController()
    {

    }

    void CameraController::Update(float dt)
    {
        if (!m_InputSystem || !m_Camera)
            return;

        if (!m_InputEnabled)
        {
            m_Camera->Update(dt);
            return;
        }

        glm::vec2 movement(0.0f);

        float scroll = m_InputSystem->GetMouseScrollY();

        if (scroll != 0.0f)
        {
            OnMouseScrolled(scroll);
        }

        if (m_InputSystem->IsKeyPressed(ANV_KEY_W))
            movement.y += 1.0f;

        if (m_InputSystem->IsKeyPressed(ANV_KEY_S))
            movement.y -= 1.0f;

        if (m_InputSystem->IsKeyPressed(ANV_KEY_D))
            movement.x += 1.0f;

        if (m_InputSystem->IsKeyPressed(ANV_KEY_A))
            movement.x -= 1.0f;

        if (movement != glm::vec2(0))
        {
            movement = glm::normalize(movement);

            m_Camera->Move(
                movement * m_Speed * m_ZoomLevel * dt);
        }

        m_Camera->Update(dt);
    }

    void CameraController::OnResize(float width, float height)
    {
        if (m_Camera && height > 0.0f)
            m_Camera->SetAspectRatio(width / height);
    }

    void CameraController::OnMouseScrolled(float yOffset)
    {
        m_ZoomLevel -= yOffset * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.1f);

        m_Camera->SetZoom(m_ZoomLevel);
    }

	void CameraController::SetCamera(_shared<Camera2D> camera)
    {
        m_Camera = camera;
    }

}

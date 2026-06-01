#include "CameraController.h"

namespace anv
{
    CameraController::CameraController(_shared<InputSystem>_is, _shared<Camera2D> _cam)
        : m_InputSystem(_is), m_Camera(_cam)
    {
        RecalculateCamera();
    }

    CameraController::~CameraController()
    {
    }

    void CameraController::Update(float dt)
    {
        glm::vec2 movement(0.0f);

        float scroll = m_InputSystem->GetMouseScrollY();

        if (scroll != 0.0f)
        {
            m_ZoomLevel -= scroll * 0.25f;
            m_ZoomLevel = std::max(m_ZoomLevel, 0.1f);

            RecalculateCamera();
        }

        if (m_InputSystem->IsKeyPressed(ANV_KEY_W))
            movement.y -= 1.0f;

        if (m_InputSystem->IsKeyPressed(ANV_KEY_S))
            movement.y += 1.0f;

        if (m_InputSystem->IsKeyPressed(ANV_KEY_D))
            movement.x += 1.0f;

        if (m_InputSystem->IsKeyPressed(ANV_KEY_A))
            movement.x -= 1.0f;

        if (movement != glm::vec2(0))
        {
            movement = glm::normalize(movement);

            m_Camera->Move(
                movement * m_Speed * dt
            );

            m_Camera->Update(dt);
        }
    }
    void CameraController::OnResize(float width, float height)
    {
        m_AspectRatio = width / height;
        RecalculateCamera();
    }

    void CameraController::OnMouseScrolled(float yOffset)
    {
        m_ZoomLevel -= yOffset * 0.25f;
        m_ZoomLevel = std::max(m_ZoomLevel, 0.1f);

        RecalculateCamera();
    }

    void CameraController::RecalculateCamera()
    {
        m_Camera->SetProjection(
            -m_AspectRatio * m_ZoomLevel,
            m_AspectRatio * m_ZoomLevel,
            -m_ZoomLevel,
            m_ZoomLevel
        );
    }
}
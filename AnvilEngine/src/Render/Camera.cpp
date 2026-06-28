#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "CameraController.h"

namespace anv
{

    // Camera2D constructor initializes the camera matrices
    Camera2D::Camera2D()
    {
        m_Transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Transform.rotation = 0.f;
        m_Transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    void Camera2D::Update(float _deltaTime)
    {
        calc_view();
    }

    void Camera2D::Move(glm::vec2 dir)
    {
        m_Transform.position += dir;
    }

    void Camera2D::calc_view()
    {
        glm::mat4 transform =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(m_Transform.position, 0.0f)
            );

        m_CameraUBO.View = glm::inverse(transform);

        m_CameraUBO.Projection = glm::ortho(
            -m_AspectRatio * m_ZoomLevel, 
            m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, 
            m_ZoomLevel, -1.0f, 1.0f);

        m_CameraUBO.ViewProjection =
            m_CameraUBO.Projection * m_CameraUBO.View;
    }

    void Camera2D::SetZoom(float _zoom)
    {
        m_ZoomLevel = _zoom;
    }

    void Camera2D::SetProjection(float left, float right, float bottom, float top)
    {
         m_CameraUBO.Projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
         calc_view();
    }

    void Camera2D::SetAspectRatio(float _ratio)
    {
        m_AspectRatio = _ratio;
        calc_view();
    }

} // namespace anv
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "CameraController.h"

namespace anv
{

    // Camera2D constructor initializes the camera matrices
    Camera2D::Camera2D()
    {
        m_Transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Transform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Transform.Scale = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    void Camera2D::Update(float _deltaTime)
    {

    }

    void Camera2D::Move(glm::vec2 dir)
    {
        m_Transform.Position += dir;

        calc_view();
        m_CameraUBO.ViewProjection =
            m_CameraUBO.Projection * m_CameraUBO.View;
    }

    void Camera2D::calc_view()
    {
        glm::mat4 transform =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(m_Transform.Position, 0.0f)
            );

        m_CameraUBO.View = glm::inverse(transform);
    }

} // namespace anv
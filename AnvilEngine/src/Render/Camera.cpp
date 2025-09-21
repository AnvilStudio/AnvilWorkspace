#include "Camera.h"

namespace anv
{

    // Camera2D constructor initializes the camera matrices
    Camera2D::Camera2D()
    {
        m_Transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Transform.Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        m_Transform.Scale = glm::vec3(1.0f, 1.0f, 1.0f);
    }

} // namespace anv
#pragma once
#include <glm/glm.hpp>
#include "Scene/Component.h"

namespace anv
{

    // class Camera3D;

    // CameraUBO is used to store camera matrices for rendering
    // It contains the model, view, and projection matrices
    struct CameraUBO
    {
        glm::mat4 Model{0.0f};
        glm::mat4 View{0.0f};
        glm::mat4 Projection{1.0f};
    };

    enum class CameraProjection
    {
        Perspective,
        Orthographic
    };
    
    class Camera2D
    {
    public:
        Camera2D();
        ~Camera2D() = default;

        CameraUBO& GetCameraUBO() { return m_CameraUBO; }

    private:
        CameraUBO m_CameraUBO;
        Component::Transform m_Transform;
    };
}
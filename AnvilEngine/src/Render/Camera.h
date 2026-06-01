#pragma once
#include <glm/glm.hpp>
#include "../Scene/Component.h"

namespace anv
{

    // class Camera3D;

    // CameraUBO is used to store camera matrices for rendering
    // It contains the model, view, and projection matrices
    struct CameraUBO
    {
        glm::mat4 View{ 1.0f };
        glm::mat4 Projection{ 1.0f };
        glm::mat4 ViewProjection{ 1.0f };
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

        void Update(float _deltaTime);
        
        // Vec2 because 2D
        void Move(glm::vec2 _dir);

        CameraUBO& GetCameraUBO() { return m_CameraUBO; }

        void SetProjection(float left, float right, float bottom, float top);

    private:
        void calc_view();

        CameraUBO m_CameraUBO;
        Component::Transform2d m_Transform;
    };
}
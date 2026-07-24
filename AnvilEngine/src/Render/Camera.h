#pragma once

#include "../Scene/Components/Transform2d.h"

#include <glm/glm.hpp>

namespace anv
{
    // CameraUBO is used to store camera matrices for rendering.
    struct CameraUBO
    {
        glm::mat4 View{1.0f};
        glm::mat4 Projection{1.0f};
        glm::mat4 ViewProjection{1.0f};
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

        CameraUBO& GetCameraUBO() { return m_CameraUBO; }
        Component::Transform2d& GetTransform() { return m_Transform; }
        float GetAspectRatio() { return m_AspectRatio; }
        float GetZoom() { return m_ZoomLevel; }

        void SetTransform(Component::Transform2d _transform) { m_Transform = _transform; }
        void SetZoom(float _zoom);
        void SetProjection(float _left, float _right, float _bottom, float _top);
        void SetAspectRatio(float _ratio);
        void Move(glm::vec2 _direction);

    private:
        void calc_view();

        CameraUBO m_CameraUBO;
        Component::Transform2d m_Transform;
        float m_AspectRatio = 16.0f / 9.0f;
        float m_ZoomLevel = 1.0f;
    };
}

#pragma once

namespace anv
{
    enum class PhysicsBodyType2D
    {
        Static = 0,
        Kinematic = 1,
        Dynamic = 2
    };

    struct PhysicsBodyDefinition2D
    {
        PhysicsBodyType2D type = PhysicsBodyType2D::Static;

        float positionX = 0.0f;
        float positionY = 0.0f;
        float rotationRadians = 0.0f;

        float linearDamping = 0.0f;
        float angularDamping = 0.0f;
        float gravityScale = 1.0f;

        bool fixedRotation = false;
        bool bullet = false;
        bool enabled = true;
    };

    struct PhysicsBoxColliderDefinition2D
    {
        float halfWidth = 0.5f;
        float halfHeight = 0.5f;

        float offsetX = 0.0f;
        float offsetY = 0.0f;

        float density = 1.0f;
        float friction = 0.5f;
        float restitution = 0.0f;

        bool sensor = false;
    };
}

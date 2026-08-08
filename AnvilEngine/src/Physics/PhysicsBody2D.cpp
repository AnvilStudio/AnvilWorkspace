#include "PhysicsBody2D.h"

#include <algorithm>

namespace anv
{
    void PhysicsBody2D::Destroy()
    {
        if (!IsValid())
            return;

        b2DestroyBody(m_Body);
        m_Body = b2_nullBodyId;
    }

    bool PhysicsBody2D::AddForce(float x, float y)
    {
        if (!IsValid())
            return false;

        b2Body_ApplyForceToCenter(m_Body, {x, y}, true);
        return true;
    }

    bool PhysicsBody2D::ApplyImpulse(float x, float y)
    {
        if (!IsValid())
            return false;

        b2Body_ApplyLinearImpulseToCenter(m_Body, {x, y}, true);
        return true;
    }

    bool PhysicsBody2D::CreateBoxCollider(
        const PhysicsBoxColliderDefinition2D& definition)
    {
        if (!IsValid())
            return false;

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = std::max(0.0f, definition.density);
        shapeDef.material.friction = std::max(0.0f, definition.friction);
        shapeDef.material.restitution =
            std::clamp(definition.restitution, 0.0f, 1.0f);
        shapeDef.isSensor = definition.sensor;
        shapeDef.enableContactEvents = true;
        shapeDef.enableSensorEvents = definition.sensor;

        const b2Polygon box = b2MakeOffsetBox(
            std::max(0.001f, definition.halfWidth),
            std::max(0.001f, definition.halfHeight),
            {definition.offsetX, definition.offsetY},
            b2MakeRot(0.0f));

        const b2ShapeId shape =
            b2CreatePolygonShape(m_Body, &shapeDef, &box);

        return !B2_IS_NULL(shape);
    }

    b2Vec2 PhysicsBody2D::GetPosition() const
    {
        return IsValid() ? b2Body_GetPosition(m_Body) : b2Vec2{0.0f, 0.0f};
    }

    float PhysicsBody2D::GetRotationRadians() const
    {
        if (!IsValid())
            return 0.0f;

        return b2Rot_GetAngle(b2Body_GetRotation(m_Body));
    }

    void PhysicsBody2D::SetTransform(float x, float y, float rotationRadians)
    {
        if (!IsValid())
            return;

        b2Body_SetTransform(m_Body, {x, y}, b2MakeRot(rotationRadians));
    }

    void PhysicsBody2D::SetAwake(bool awake)
    {
        if (!IsValid())
            return;

        b2Body_SetAwake(m_Body, awake);
    }
}

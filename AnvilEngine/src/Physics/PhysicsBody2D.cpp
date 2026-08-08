#include "PhysicsBody2D.h"

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

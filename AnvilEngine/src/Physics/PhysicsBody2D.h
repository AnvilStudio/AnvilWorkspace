#pragma once

#include "PhysicsTypes.h"

#include <box2d/box2d.h>

namespace anv
{
    class PhysicsBody2D
    {
    public:
        PhysicsBody2D() = default;
        explicit PhysicsBody2D(b2BodyId body)
            : m_Body(body)
        {
        }

        bool IsValid() const { return !B2_IS_NULL(m_Body); }
        b2BodyId GetNativeBody() const { return m_Body; }

        void Destroy();

        bool AddForce(float x, float y);
        bool ApplyImpulse(float x, float y);
        bool CreateBoxCollider(const PhysicsBoxColliderDefinition2D& definition);

        b2Vec2 GetPosition() const;
        float GetRotationRadians() const;
        void SetTransform(float x, float y, float rotationRadians);
        void SetAwake(bool awake);

    private:
        b2BodyId m_Body = b2_nullBodyId;
    };
}

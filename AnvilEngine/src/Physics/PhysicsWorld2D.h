#pragma once

#include <box2d/box2d.h>

namespace anv
{
    class PhysicsWorld2D
    {
    public:
        PhysicsWorld2D() = default;
        ~PhysicsWorld2D();

        PhysicsWorld2D(const PhysicsWorld2D&) = delete;
        PhysicsWorld2D& operator=(const PhysicsWorld2D&) = delete;

        bool Create(float gravityX, float gravityY);
        void Destroy();
        void Step(float timeStep, int subStepCount);

        bool IsValid() const { return !B2_IS_NULL(m_World); }
        b2WorldId GetNativeWorld() const { return m_World; }

    private:
        b2WorldId m_World = b2_nullWorldId;
    };
}

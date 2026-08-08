#pragma once

#include "PhysicsBody2D.h"
#include "PhysicsTypes.h"

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

        PhysicsBody2D CreateBody(const PhysicsBodyDefinition2D& definition);
        void DestroyBody(PhysicsBody2D& body);

        bool IsValid() const { return !B2_IS_NULL(m_World); }
        b2WorldId GetNativeWorld() const { return m_World; }

    private:
        static b2BodyType ToBox2DType(PhysicsBodyType2D type);

    private:
        b2WorldId m_World = b2_nullWorldId;
    };
}

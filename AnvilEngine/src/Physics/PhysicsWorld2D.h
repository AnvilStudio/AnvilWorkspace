#pragma once

#include "PhysicsBody2D.h"
#include "PhysicsTypes.h"
#include "CollisionEvents2D.h"

#include <box2d/box2d.h>
#include <vector>

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

        // Drain after each fixed step; events are not preserved by Box2D across steps.
        std::vector<PhysicsCollisionEvent2D> ConsumeCollisionEvents();

        PhysicsBody2D CreateBody(const PhysicsBodyDefinition2D& definition);
        void DestroyBody(PhysicsBody2D& body);

        bool IsValid() const { return !B2_IS_NULL(m_World); }
        b2WorldId GetNativeWorld() const { return m_World; }

    private:
        static b2BodyType ToBox2DType(PhysicsBodyType2D type);

    private:
        b2WorldId m_World = b2_nullWorldId;
        std::vector<PhysicsCollisionEvent2D> m_CollisionEvents;
    };
}

#include "PhysicsWorld2D.h"

namespace anv
{
    PhysicsWorld2D::~PhysicsWorld2D()
    {
        Destroy();
    }

    bool PhysicsWorld2D::Create(float gravityX, float gravityY)
    {
        Destroy();

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {gravityX, gravityY};

        m_World = b2CreateWorld(&worldDef);
        return IsValid();
    }

    void PhysicsWorld2D::Destroy()
    {
        if (!IsValid())
            return;

        b2DestroyWorld(m_World);
        m_World = b2_nullWorldId;
    }

    void PhysicsWorld2D::Step(float timeStep, int subStepCount)
    {
        if (!IsValid())
            return;

        b2World_Step(m_World, timeStep, subStepCount);
    }
}

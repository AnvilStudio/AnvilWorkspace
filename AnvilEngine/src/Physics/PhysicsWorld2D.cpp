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

    PhysicsBody2D PhysicsWorld2D::CreateBody(
        const PhysicsBodyDefinition2D& definition)
    {
        if (!IsValid())
            return {};

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = ToBox2DType(definition.type);
        bodyDef.position = {definition.positionX, definition.positionY};
        bodyDef.rotation = b2MakeRot(definition.rotationRadians);
        bodyDef.linearDamping = definition.linearDamping;
        bodyDef.angularDamping = definition.angularDamping;
        bodyDef.gravityScale = definition.gravityScale;
        bodyDef.motionLocks.linearX = false;
        bodyDef.motionLocks.linearY = false;
        bodyDef.motionLocks.angularZ = definition.fixedRotation;
        bodyDef.isBullet = definition.bullet;
        bodyDef.isEnabled = definition.enabled;
        bodyDef.enableSleep = true;
        bodyDef.isAwake = true;

        return PhysicsBody2D(b2CreateBody(m_World, &bodyDef));
    }

    void PhysicsWorld2D::DestroyBody(PhysicsBody2D& body)
    {
        body.Destroy();
    }

    b2BodyType PhysicsWorld2D::ToBox2DType(PhysicsBodyType2D type)
    {
        switch (type)
        {
            case PhysicsBodyType2D::Static:
                return b2_staticBody;
            case PhysicsBodyType2D::Kinematic:
                return b2_kinematicBody;
            case PhysicsBodyType2D::Dynamic:
                return b2_dynamicBody;
        }

        return b2_staticBody;
    }
}

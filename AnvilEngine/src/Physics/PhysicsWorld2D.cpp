#include "PhysicsWorld2D.h"

#include <utility>

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
        m_CollisionEvents.clear();
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

        // Copy immediately: Box2D event buffers are only valid until the next step.
        const b2ContactEvents contacts = b2World_GetContactEvents(m_World);
        for (int i = 0; i < contacts.beginCount; ++i)
        {
            const auto& event = contacts.beginEvents[i];
            if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB))
                continue;
            m_CollisionEvents.push_back({
                b2Shape_GetBody(event.shapeIdA), b2Shape_GetBody(event.shapeIdB),
                CollisionEventPhase2D::Begin, CollisionEventKind2D::Contact});
        }
        for (int i = 0; i < contacts.endCount; ++i)
        {
            const auto& event = contacts.endEvents[i];
            // Box2D can report an end event for a shape destroyed during the step.
            if (!b2Shape_IsValid(event.shapeIdA) || !b2Shape_IsValid(event.shapeIdB))
                continue;
            m_CollisionEvents.push_back({
                b2Shape_GetBody(event.shapeIdA), b2Shape_GetBody(event.shapeIdB),
                CollisionEventPhase2D::End, CollisionEventKind2D::Contact});
        }

        const b2SensorEvents sensors = b2World_GetSensorEvents(m_World);
        for (int i = 0; i < sensors.beginCount; ++i)
        {
            const auto& event = sensors.beginEvents[i];
            if (!b2Shape_IsValid(event.sensorShapeId) || !b2Shape_IsValid(event.visitorShapeId))
                continue;
            m_CollisionEvents.push_back({
                b2Shape_GetBody(event.sensorShapeId), b2Shape_GetBody(event.visitorShapeId),
                CollisionEventPhase2D::Begin, CollisionEventKind2D::Sensor});
        }
        for (int i = 0; i < sensors.endCount; ++i)
        {
            const auto& event = sensors.endEvents[i];
            if (!b2Shape_IsValid(event.sensorShapeId) || !b2Shape_IsValid(event.visitorShapeId))
                continue;
            m_CollisionEvents.push_back({
                b2Shape_GetBody(event.sensorShapeId), b2Shape_GetBody(event.visitorShapeId),
                CollisionEventPhase2D::End, CollisionEventKind2D::Sensor});
        }
    }

    std::vector<PhysicsCollisionEvent2D> PhysicsWorld2D::ConsumeCollisionEvents()
    {
        return std::exchange(m_CollisionEvents, {});
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

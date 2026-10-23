#include "../Util/UMacros.h"
#include <Core/App.h>
#include <Scene/Component.h>
#include "CollisionListener.h"
#include "../vendor/entt/single_include/entt/entt.hpp"
#include <Scripting/PythonScriptEngine.h>

namespace anv
{
    b2ContactEvents CollisionListener::m_WorldContactEvents {};
    b2SensorEvents CollisionListener::m_WorldSensorEvents = {};
    Ref<Scene> CollisionListener::m_CurrentScene = nullptr;


    // call b2World_GetContactEvents() to get the events
    void CollisionListener::PollCollision(b2WorldId _world)
    {
        m_WorldContactEvents = b2World_GetContactEvents(_world);
        m_WorldSensorEvents = b2World_GetSensorEvents(_world);


        // ensure current scene is valid before dispatching
        m_CurrentScene = App::GetInstance()->GetSceneManager()->GetActive();

        if (!m_CurrentScene)
            return;

        BeginCollision();
        EndCollision();
        BeginSensor();
        EndSensor();
    }

    void CollisionListener::BeginCollision()
    {
        // Handle both Hit and Collision events
        for (int i = 0; i < m_WorldContactEvents.beginCount; ++i)
        {
            b2ContactBeginTouchEvent *event = m_WorldContactEvents.beginEvents + i;
            PhysicsCollisionEvent2D newEvent{};
            newEvent.shapeA = event->shapeIdA;
            newEvent.shapeB = event->shapeIdB;
            newEvent.kind = CollisionEventKind2D::Contact;
            newEvent.phase = CollisionEventPhase2D::Begin;

            if (!dispatch_event(newEvent))
            {
                ANV_LOG_ERROR("Failed to dispatch Physics2DCollisionEvent!")
            }
        }
    }

    void CollisionListener::EndCollision()
    {
        // Handle both Hit and Collision events
        for (int i = 0; i < m_WorldContactEvents.endCount; ++i)
        {
            b2ContactEndTouchEvent *event = m_WorldContactEvents.endEvents + i;
            PhysicsCollisionEvent2D newEvent{};
            newEvent.shapeA = event->shapeIdA;
            newEvent.shapeB = event->shapeIdB;
            newEvent.kind = CollisionEventKind2D::Contact;
            newEvent.phase = CollisionEventPhase2D::End;

            dispatch_event(newEvent);
        }
    }

    void CollisionListener::BeginSensor()
    {
        // Handle sensor events
        for (int i = 0; i < m_WorldSensorEvents.beginCount; ++i)
        {
            ANV_LOG_DEBUG("Sensor Collided");
            b2SensorBeginTouchEvent *event = m_WorldSensorEvents.beginEvents + i;
            PhysicsCollisionEvent2D newEvent{};
            newEvent.shapeA = event->sensorShapeId;
            newEvent.shapeB = event->visitorShapeId;
            newEvent.kind = CollisionEventKind2D::Sensor;
            newEvent.phase = CollisionEventPhase2D::Begin;

            dispatch_event(newEvent);
        }
    }

    void CollisionListener::EndSensor()
    {
        // Handle sensor end events
        for (int i = 0; i < m_WorldSensorEvents.endCount; ++i)
        {
            b2SensorEndTouchEvent *event = m_WorldSensorEvents.endEvents + i;
            PhysicsCollisionEvent2D newEvent{};
            newEvent.shapeA = event->sensorShapeId;
            newEvent.shapeB = event->visitorShapeId;
            newEvent.kind = CollisionEventKind2D::Sensor;
            newEvent.phase = CollisionEventPhase2D::End;

            dispatch_event(newEvent);
        }
    }

    // TODO: Pass in the event kind and phase
    bool CollisionListener::dispatch_event(PhysicsCollisionEvent2D& _event2D)
    {
        // resolve body A and body B to engine entities, then invoke OnCollision functions
        
        if (!m_CurrentScene)
            return false;

        // retrieve the bodies that own the shapes
        b2BodyId bodyA = b2Shape_GetBody(_event2D.shapeA);
        b2BodyId bodyB = b2Shape_GetBody(_event2D.shapeB);

        // Bodies user data is set to the entity, retrieve the entity handle
        void *userA = b2Body_GetUserData(bodyA);
        void *userB = b2Body_GetUserData(bodyB);

        if (!userA || !userB)
            return false;

        entt::entity entityA = static_cast<entt::entity>(
            reinterpret_cast<uintptr_t>(userA));

        entt::entity entityB = static_cast<entt::entity>(
            reinterpret_cast<uintptr_t>(userB));

        if (m_CurrentScene->HasComponent<Component::Script>(entityA))
        {
            PythonScriptEngine::InvokeOnCollision(*m_CurrentScene, entityA, entityB, _event2D.kind, _event2D.phase);
        }

        if (m_CurrentScene->HasComponent<Component::Script>(entityB))
        {
            PythonScriptEngine::InvokeOnCollision(*m_CurrentScene, entityB, entityA, _event2D.kind, _event2D.phase);
        }

        return true;
    }
}
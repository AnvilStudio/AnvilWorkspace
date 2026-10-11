#pragma once
#include "CollisionEvents2D.h"
#include <Scene/Scene.h>
#include <box2d/box2d.h>

namespace anv
{
    class CollisionListener
    {
        public:
            // only call after physics step 
            static void PollCollision(b2WorldId _world);

        private:

            // Build specific engine level collision events and dispatch them.
            static void BeginCollision();
            static void EndCollision();
            static void BeginSensor();
            static void EndSensor();

            //static void resolve_entity_from_shape(b2ShapeId _shape);

            // return false if dispatch failed
            static bool dispatch_event(PhysicsCollisionEvent2D& _event2D);

        private:
            static b2ContactEvents m_WorldContactEvents;
            static b2SensorEvents m_WorldSensorEvents;
            static Ref<Scene> m_CurrentScene;
    };
}
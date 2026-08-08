#pragma once

#include "PhysicsWorld2D.h"
#include "../Core/Reference.h"
#include "../vendor/entt/single_include/entt/entt.hpp"

#include <box2d/box2d.h>

#include <unordered_map>

namespace anv
{
    class Scene;

    class Physics2D
    {
    public:
        Physics2D() = default;
        ~Physics2D();

        Physics2D(const Physics2D&) = delete;
        Physics2D& operator=(const Physics2D&) = delete;

        bool Start(Scene& scene);
        void Stop();
        void Step(Scene& scene, float deltaTime);

        void DestroyBody(entt::entity entity);
        bool IsRunning() const { return m_Running; }

        bool AddForce(entt::entity entity, float x, float y)
        {
            const auto it = m_Bodies.find(entity);
            if (!m_Running || it == m_Bodies.end())
                return false;

            b2Body_ApplyForceToCenter(it->second, {x, y}, true);
            return true;
        }

        bool ApplyImpulse(entt::entity entity, float x, float y)
        {
            const auto it = m_Bodies.find(entity);
            if (!m_Running || it == m_Bodies.end())
                return false;

            b2Body_ApplyLinearImpulseToCenter(it->second, {x, y}, true);
            return true;
        }

    private:
        void CreateBody(Scene& scene, entt::entity entity);
        void RemoveDestroyedBodies(Scene& scene);
        void SynchronizeBodiesFromTransforms(Scene& scene);
        void SynchronizeTransforms(Scene& scene);

        static b2BodyType ToBox2DType(int type);

    private:
        PhysicsWorld2D m_World;
        std::unordered_map<entt::entity, b2BodyId> m_Bodies;

        float m_Accumulator = 0.0f;
        float m_FixedTimeStep = 1.0f / 60.0f;
        int m_SubStepCount = 4;
        bool m_Running = false;
    };
}

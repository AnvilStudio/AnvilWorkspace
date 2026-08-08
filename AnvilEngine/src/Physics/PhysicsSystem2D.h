#pragma once

#include "PhysicsBody2D.h"
#include "PhysicsWorld2D.h"
#include "../Core/Reference.h"
#include "../vendor/entt/single_include/entt/entt.hpp"

#include <unordered_map>

namespace anv
{
    class Scene;

    class PhysicsSystem2D
    {
    public:
        PhysicsSystem2D() = default;
        ~PhysicsSystem2D();

        PhysicsSystem2D(const PhysicsSystem2D&) = delete;
        PhysicsSystem2D& operator=(const PhysicsSystem2D&) = delete;

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

            return it->second.AddForce(x, y);
        }

        bool ApplyImpulse(entt::entity entity, float x, float y)
        {
            const auto it = m_Bodies.find(entity);
            if (!m_Running || it == m_Bodies.end())
                return false;

            return it->second.ApplyImpulse(x, y);
        }

    private:
        void CreateBody(Scene& scene, entt::entity entity);
        void RemoveDestroyedBodies(Scene& scene);
        void SynchronizeBodiesFromTransforms(Scene& scene);
        void SynchronizeTransforms(Scene& scene);

    private:
        PhysicsWorld2D m_World;
        std::unordered_map<entt::entity, PhysicsBody2D> m_Bodies;

        float m_Accumulator = 0.0f;
        float m_FixedTimeStep = 1.0f / 60.0f;
        int m_SubStepCount = 4;
        bool m_Running = false;
    };
}

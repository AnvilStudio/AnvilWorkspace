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

        PhysicsBody2D* GetBody(entt::entity entity)
        {
            if (!m_Running)
                return nullptr;

            const auto it = m_Bodies.find(entity);
            return it != m_Bodies.end() ? &it->second.body : nullptr;
        }

        const PhysicsBody2D* GetBody(entt::entity entity) const
        {
            if (!m_Running)
                return nullptr;

            const auto it = m_Bodies.find(entity);
            return it != m_Bodies.end() ? &it->second.body : nullptr;
        }

    private:
        struct RuntimeBody2D
        {
            PhysicsBody2D body;
            PhysicsTransform2D previousTransform;
            PhysicsTransform2D currentTransform;
        };

        void CreateBody(Scene& scene, entt::entity entity);
        void RemoveDestroyedBodies(Scene& scene);
        void SynchronizeBodiesFromTransforms(Scene& scene);
        void CapturePreStepTransforms(Scene& scene);
        void CapturePostStepTransforms(Scene& scene);
        void SynchronizeTransforms(Scene& scene);

    private:
        PhysicsWorld2D m_World;
        std::unordered_map<entt::entity, RuntimeBody2D> m_Bodies;

        float m_Accumulator = 0.0f;
        float m_FixedTimeStep = 1.0f / 60.0f;
        int m_SubStepCount = 4;
        bool m_Running = false;
    };
}

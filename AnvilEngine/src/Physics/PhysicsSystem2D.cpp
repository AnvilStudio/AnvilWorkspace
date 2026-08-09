#include "PhysicsSystem2D.h"

#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../Util/AnvLog/AnvLog.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace anv
{
    namespace
    {
        const char* BodyTypeName(Component::Rigidbody2DType type)
        {
            switch (type)
            {
                case Component::Rigidbody2DType::Static: return "Static";
                case Component::Rigidbody2DType::Kinematic: return "Kinematic";
                case Component::Rigidbody2DType::Dynamic: return "Dynamic";
            }

            return "Unknown";
        }

        PhysicsBodyType2D ToPhysicsBodyType(Component::Rigidbody2DType type)
        {
            switch (type)
            {
                case Component::Rigidbody2DType::Static:
                    return PhysicsBodyType2D::Static;
                case Component::Rigidbody2DType::Kinematic:
                    return PhysicsBodyType2D::Kinematic;
                case Component::Rigidbody2DType::Dynamic:
                    return PhysicsBodyType2D::Dynamic;
            }

            return PhysicsBodyType2D::Static;
        }

        bool NearlyEqual(float left, float right, float epsilon = 0.0001f)
        {
            return std::abs(left - right) <= epsilon;
        }
    }

    PhysicsSystem2D::~PhysicsSystem2D()
    {
        Stop();
    }

    bool PhysicsSystem2D::Start(Scene& scene)
    {
        Stop();

        if (!m_World.Create(0.0f, -9.81f))
        {
            ANV_LOG_ERROR("PhysicsSystem2D failed to create its physics world.");
            return false;
        }

        m_Running = true;
        m_Accumulator = 0.0f;

        auto view = scene.Registry().view<Component::Transform2d>();
        std::size_t dynamicBodyCount = 0;
        std::size_t colliderOnlyBodyCount = 0;

        for (const auto entity : view)
        {
            const bool hasRigidbody =
                scene.HasComponent<Component::Rigidbody2D>(entity);
            const bool hasCollider =
                scene.HasComponent<Component::BoxCollider2D>(entity);

            if (!hasRigidbody && !hasCollider)
                continue;

            if (hasRigidbody)
            {
                const auto& rigidbody =
                    scene.GetComponent<Component::Rigidbody2D>(entity);

                if (rigidbody.type == Component::Rigidbody2DType::Dynamic)
                    ++dynamicBodyCount;
            }
            else
            {
                ++colliderOnlyBodyCount;
            }

            CreateBody(scene, entity);
        }

        ANV_LOG_INFO(
            "PhysicsSystem2D started with %zu bodies (%zu dynamic, %zu implicit static colliders).",
            m_Bodies.size(),
            dynamicBodyCount,
            colliderOnlyBodyCount);

        if (dynamicBodyCount == 0)
        {
            ANV_LOG_WARN(
                "PhysicsSystem2D has no dynamic bodies. Static and kinematic bodies do not fall under gravity.");
        }

        return true;
    }

    void PhysicsSystem2D::Stop()
    {
        if (!m_Running)
            return;

        m_Bodies.clear();
        m_World.Destroy();
        m_Accumulator = 0.0f;
        m_Running = false;

        ANV_LOG_INFO("PhysicsSystem2D stopped.");
    }

    void PhysicsSystem2D::Step(Scene& scene, float deltaTime)
    {
        if (!m_Running)
            return;

        auto view = scene.Registry().view<Component::Transform2d>();

        for (const auto entity : view)
        {
            const bool participatesInPhysics =
                scene.HasComponent<Component::Rigidbody2D>(entity) ||
                scene.HasComponent<Component::BoxCollider2D>(entity);

            if (participatesInPhysics && !m_Bodies.contains(entity))
                CreateBody(scene, entity);
        }

        RemoveDestroyedBodies(scene);
        SynchronizeBodiesFromTransforms(scene);

        m_Accumulator += std::clamp(deltaTime, 0.0f, 0.25f);

        while (m_Accumulator >= m_FixedTimeStep)
        {
            m_World.Step(m_FixedTimeStep, m_SubStepCount);
            m_Accumulator -= m_FixedTimeStep;
        }

        SynchronizeTransforms(scene);
    }

    void PhysicsSystem2D::DestroyBody(entt::entity entity)
    {
        const auto it = m_Bodies.find(entity);
        if (it == m_Bodies.end())
            return;

        m_World.DestroyBody(it->second);
        m_Bodies.erase(it);
    }

    void PhysicsSystem2D::CreateBody(Scene& scene, entt::entity entity)
    {
        if (!m_Running ||
            !scene.Registry().valid(entity) ||
            m_Bodies.contains(entity) ||
            !scene.HasComponent<Component::Transform2d>(entity))
        {
            return;
        }

        const bool hasRigidbody =
            scene.HasComponent<Component::Rigidbody2D>(entity);
        const bool hasCollider =
            scene.HasComponent<Component::BoxCollider2D>(entity);

        if (!hasRigidbody && !hasCollider)
            return;

        const auto& transform =
            scene.GetComponent<Component::Transform2d>(entity);

        PhysicsBodyDefinition2D bodyDefinition;
        bodyDefinition.positionX = transform.position.x;
        bodyDefinition.positionY = transform.position.y;
        bodyDefinition.rotationRadians = glm::radians(transform.rotation);

        Component::Rigidbody2DType engineBodyType =
            Component::Rigidbody2DType::Static;

        if (hasRigidbody)
        {
            const auto& rigidbody =
                scene.GetComponent<Component::Rigidbody2D>(entity);

            engineBodyType = rigidbody.type;
            bodyDefinition.type = ToPhysicsBodyType(rigidbody.type);
            bodyDefinition.linearDamping = std::max(0.0f, rigidbody.linearDamping);
            bodyDefinition.angularDamping = std::max(0.0f, rigidbody.angularDamping);
            bodyDefinition.gravityScale = rigidbody.gravityScale;
            bodyDefinition.fixedRotation = rigidbody.fixedRotation;
            bodyDefinition.bullet = rigidbody.bullet;
            bodyDefinition.enabled = rigidbody.enabled;
        }
        else
        {
            bodyDefinition.type = PhysicsBodyType2D::Static;
            bodyDefinition.gravityScale = 0.0f;
            bodyDefinition.enabled = true;
        }

        PhysicsBody2D body = m_World.CreateBody(bodyDefinition);
        if (!body.IsValid())
        {
            ANV_LOG_ERROR(
                "PhysicsSystem2D failed to create a runtime body for entity %u.",
                static_cast<unsigned int>(entity));
            return;
        }

        auto [bodyIterator, inserted] = m_Bodies.emplace(entity, body);
        if (!inserted)
        {
            m_World.DestroyBody(body);
            return;
        }

        PhysicsBody2D& runtimeBody = bodyIterator->second;

        ANV_LOG_INFO(
            "PhysicsSystem2D body created: entity=%u type=%s position=(%.3f, %.3f) collider=%s",
            static_cast<unsigned int>(entity),
            BodyTypeName(engineBodyType),
            transform.position.x,
            transform.position.y,
            hasCollider ? "true" : "false");

        if (!hasCollider)
        {
            ANV_LOG_WARN(
                "PhysicsSystem2D entity %u has a Rigidbody2D but no BoxCollider2D; it can move but cannot collide.",
                static_cast<unsigned int>(entity));
            return;
        }

        const auto& collider =
            scene.GetComponent<Component::BoxCollider2D>(entity);

        const float absoluteScaleX = std::abs(transform.scale.x);
        const float absoluteScaleY = std::abs(transform.scale.y);

        PhysicsBoxColliderDefinition2D colliderDefinition;
        colliderDefinition.halfWidth = std::max(
            0.001f,
            std::abs(collider.size.x) * absoluteScaleX * 0.5f);
        colliderDefinition.halfHeight = std::max(
            0.001f,
            std::abs(collider.size.y) * absoluteScaleY * 0.5f);
        colliderDefinition.offsetX = collider.offset.x * transform.scale.x;
        colliderDefinition.offsetY = collider.offset.y * transform.scale.y;
        colliderDefinition.density = std::max(0.0f, collider.density);
        colliderDefinition.friction = std::max(0.0f, collider.friction);
        colliderDefinition.restitution =
            std::clamp(collider.restitution, 0.0f, 1.0f);
        colliderDefinition.sensor = collider.sensor;

        if (!runtimeBody.CreateBoxCollider(colliderDefinition))
        {
            ANV_LOG_ERROR(
                "PhysicsSystem2D failed to create BoxCollider2D shape for entity %u.",
                static_cast<unsigned int>(entity));
        }
        else
        {
            ANV_LOG_INFO(
                "PhysicsSystem2D collider created: entity=%u halfExtents=(%.3f, %.3f) sensor=%s",
                static_cast<unsigned int>(entity),
                colliderDefinition.halfWidth,
                colliderDefinition.halfHeight,
                collider.sensor ? "true" : "false");
        }
    }

    void PhysicsSystem2D::RemoveDestroyedBodies(Scene& scene)
    {
        std::vector<entt::entity> staleEntities;

        for (const auto& [entity, body] : m_Bodies)
        {
            (void)body;

            const bool stillParticipates =
                scene.Registry().valid(entity) &&
                scene.HasComponent<Component::Transform2d>(entity) &&
                (scene.HasComponent<Component::Rigidbody2D>(entity) ||
                 scene.HasComponent<Component::BoxCollider2D>(entity));

            if (!stillParticipates)
                staleEntities.push_back(entity);
        }

        for (const auto entity : staleEntities)
            DestroyBody(entity);
    }

    void PhysicsSystem2D::SynchronizeBodiesFromTransforms(Scene& scene)
    {
        for (auto& [entity, body] : m_Bodies)
        {
            if (!body.IsValid() ||
                !scene.Registry().valid(entity) ||
                !scene.HasComponent<Component::Transform2d>(entity))
            {
                continue;
            }

            if (scene.HasComponent<Component::Rigidbody2D>(entity))
            {
                const auto& rigidbody =
                    scene.GetComponent<Component::Rigidbody2D>(entity);

                if (rigidbody.type == Component::Rigidbody2DType::Dynamic)
                    continue;
            }

            const auto& transform =
                scene.GetComponent<Component::Transform2d>(entity);

            const PhysicsTransform2D bodyTransform = body.GetTransform();
            const float bodyRotation = glm::degrees(bodyTransform.rotationRadians);

            const bool positionChanged =
                !NearlyEqual(transform.position.x, bodyTransform.positionX) ||
                !NearlyEqual(transform.position.y, bodyTransform.positionY);

            const bool rotationChanged =
                !NearlyEqual(transform.rotation, bodyRotation);

            if (!positionChanged && !rotationChanged)
                continue;

            body.SetTransform(
                transform.position.x,
                transform.position.y,
                glm::radians(transform.rotation));
            body.SetAwake(true);
        }
    }

    void PhysicsSystem2D::SynchronizeTransforms(Scene& scene)
    {
        for (const auto& [entity, body] : m_Bodies)
        {
            if (!body.IsValid() ||
                !scene.Registry().valid(entity) ||
                !scene.HasComponent<Component::Transform2d>(entity) ||
                !scene.HasComponent<Component::Rigidbody2D>(entity))
            {
                continue;
            }

            const auto& rigidbody =
                scene.GetComponent<Component::Rigidbody2D>(entity);

            if (rigidbody.type == Component::Rigidbody2DType::Static)
                continue;

            auto& transform =
                scene.GetComponent<Component::Transform2d>(entity);

            const PhysicsTransform2D bodyTransform = body.GetTransform();
            transform.position = {
                bodyTransform.positionX,
                bodyTransform.positionY
            };
            transform.rotation = glm::degrees(bodyTransform.rotationRadians);
        }
    }
}

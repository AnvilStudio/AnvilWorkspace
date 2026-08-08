#include "Physics2D.h"

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

        bool NearlyEqual(float left, float right, float epsilon = 0.0001f)
        {
            return std::abs(left - right) <= epsilon;
        }
    }

    Physics2D::~Physics2D()
    {
        Stop();
    }

    bool Physics2D::Start(Scene& scene)
    {
        Stop();

        if (!m_World.Create(0.0f, -9.81f))
        {
            ANV_LOG_ERROR("Physics2D failed to create its physics world.");
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
            "Physics2D started with %zu bodies (%zu dynamic, %zu implicit static colliders).",
            m_Bodies.size(),
            dynamicBodyCount,
            colliderOnlyBodyCount);

        if (dynamicBodyCount == 0)
        {
            ANV_LOG_WARN(
                "Physics2D has no dynamic bodies. Static and kinematic bodies do not fall under gravity.");
        }

        return true;
    }

    void Physics2D::Stop()
    {
        if (!m_Running)
            return;

        m_Bodies.clear();
        m_World.Destroy();
        m_Accumulator = 0.0f;
        m_Running = false;

        ANV_LOG_INFO("Physics2D stopped.");
    }

    void Physics2D::Step(Scene& scene, float deltaTime)
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

    void Physics2D::DestroyBody(entt::entity entity)
    {
        const auto it = m_Bodies.find(entity);
        if (it == m_Bodies.end())
            return;

        it->second.Destroy();
        m_Bodies.erase(it);
    }

    void Physics2D::CreateBody(Scene& scene, entt::entity entity)
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

        b2BodyDef bodyDef = b2DefaultBodyDef();
        Component::Rigidbody2DType engineBodyType =
            Component::Rigidbody2DType::Static;

        if (hasRigidbody)
        {
            const auto& rigidbody =
                scene.GetComponent<Component::Rigidbody2D>(entity);

            engineBodyType = rigidbody.type;
            bodyDef.type = ToBox2DType(static_cast<int>(rigidbody.type));
            bodyDef.linearDamping = std::max(0.0f, rigidbody.linearDamping);
            bodyDef.angularDamping = std::max(0.0f, rigidbody.angularDamping);
            bodyDef.gravityScale = rigidbody.gravityScale;
            bodyDef.motionLocks.linearX = false;
            bodyDef.motionLocks.linearY = false;
            bodyDef.motionLocks.angularZ = rigidbody.fixedRotation;
            bodyDef.isBullet = rigidbody.bullet;
            bodyDef.isEnabled = rigidbody.enabled;
        }
        else
        {
            bodyDef.type = b2_staticBody;
            bodyDef.gravityScale = 0.0f;
            bodyDef.isEnabled = true;
        }

        bodyDef.position = {transform.position.x, transform.position.y};
        bodyDef.rotation = b2MakeRot(glm::radians(transform.rotation));
        bodyDef.enableSleep = true;
        bodyDef.isAwake = true;

        const b2BodyId nativeBody = b2CreateBody(m_World.GetNativeWorld(), &bodyDef);
        auto [bodyIterator, inserted] = m_Bodies.emplace(entity, PhysicsBody2D(nativeBody));
        if (!inserted)
            return;

        PhysicsBody2D& body = bodyIterator->second;

        ANV_LOG_INFO(
            "Physics2D body created: entity=%u type=%s position=(%.3f, %.3f) collider=%s",
            static_cast<unsigned int>(entity),
            BodyTypeName(engineBodyType),
            transform.position.x,
            transform.position.y,
            hasCollider ? "true" : "false");

        if (!hasCollider)
        {
            ANV_LOG_WARN(
                "Physics2D entity %u has a Rigidbody2D but no BoxCollider2D; it can move but cannot collide.",
                static_cast<unsigned int>(entity));
            return;
        }

        const auto& collider =
            scene.GetComponent<Component::BoxCollider2D>(entity);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = std::max(0.0f, collider.density);
        shapeDef.material.friction = std::max(0.0f, collider.friction);
        shapeDef.material.restitution =
            std::clamp(collider.restitution, 0.0f, 1.0f);
        shapeDef.isSensor = collider.sensor;
        shapeDef.enableContactEvents = true;
        shapeDef.enableSensorEvents = collider.sensor;

        const float absoluteScaleX = std::abs(transform.scale.x);
        const float absoluteScaleY = std::abs(transform.scale.y);

        const float halfWidth = std::max(
            0.001f,
            std::abs(collider.size.x) * absoluteScaleX * 0.5f);

        const float halfHeight = std::max(
            0.001f,
            std::abs(collider.size.y) * absoluteScaleY * 0.5f);

        const b2Vec2 scaledOffset{
            collider.offset.x * transform.scale.x,
            collider.offset.y * transform.scale.y
        };

        const b2Polygon box = b2MakeOffsetBox(
            halfWidth,
            halfHeight,
            scaledOffset,
            b2MakeRot(0.0f));

        const b2ShapeId shape =
            b2CreatePolygonShape(body.GetNativeBody(), &shapeDef, &box);

        if (B2_IS_NULL(shape))
        {
            ANV_LOG_ERROR(
                "Physics2D failed to create BoxCollider2D shape for entity %u.",
                static_cast<unsigned int>(entity));
        }
        else
        {
            ANV_LOG_INFO(
                "Physics2D collider created: entity=%u halfExtents=(%.3f, %.3f) sensor=%s",
                static_cast<unsigned int>(entity),
                halfWidth,
                halfHeight,
                collider.sensor ? "true" : "false");
        }
    }

    void Physics2D::RemoveDestroyedBodies(Scene& scene)
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

    void Physics2D::SynchronizeBodiesFromTransforms(Scene& scene)
    {
        for (auto& [entity, body] : m_Bodies)
        {
            if (!body.IsValid() ||
                !scene.Registry().valid(entity) ||
                !scene.HasComponent<Component::Transform2d>(entity))
            {
                continue;
            }

            const auto& transform =
                scene.GetComponent<Component::Transform2d>(entity);

            const b2Vec2 bodyPosition = body.GetPosition();
            const float bodyRotation = glm::degrees(body.GetRotationRadians());

            const bool positionChanged =
                !NearlyEqual(transform.position.x, bodyPosition.x) ||
                !NearlyEqual(transform.position.y, bodyPosition.y);

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

    void Physics2D::SynchronizeTransforms(Scene& scene)
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

            const b2Vec2 position = body.GetPosition();
            transform.position = {position.x, position.y};
            transform.rotation = glm::degrees(body.GetRotationRadians());
        }
    }

    b2BodyType Physics2D::ToBox2DType(int type)
    {
        switch (static_cast<Component::Rigidbody2DType>(type))
        {
            case Component::Rigidbody2DType::Static:
                return b2_staticBody;
            case Component::Rigidbody2DType::Kinematic:
                return b2_kinematicBody;
            case Component::Rigidbody2DType::Dynamic:
                return b2_dynamicBody;
        }

        return b2_staticBody;
    }
}

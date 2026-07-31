#include "Physics2D.h"

#include "../Scene/Component.h"
#include "../Scene/Scene.h"
#include "../Util/AnvLog/AnvLog.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace anv
{
    Physics2D::~Physics2D()
    {
        Stop();
    }

    bool Physics2D::Start(Scene& scene)
    {
        Stop();

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {0.0f, -9.81f};

        m_World = b2CreateWorld(&worldDef);
        m_Running = true;
        m_Accumulator = 0.0f;

        auto view = scene.Registry().view<
            Component::Transform2d,
            Component::Rigidbody2D>();

        for (const auto entity : view)
            CreateBody(scene, entity);

        ANV_LOG_INFO("Physics2D started with %zu bodies.", m_Bodies.size());
        return true;
    }

    void Physics2D::Stop()
    {
        if (!m_Running)
            return;

        m_Bodies.clear();
        b2DestroyWorld(m_World);
        m_World = b2_nullWorldId;
        m_Accumulator = 0.0f;
        m_Running = false;

        ANV_LOG_INFO("Physics2D stopped.");
    }

    void Physics2D::Step(Scene& scene, float deltaTime)
    {
        if (!m_Running)
            return;

        auto view = scene.Registry().view<
            Component::Transform2d,
            Component::Rigidbody2D>();

        for (const auto entity : view)
        {
            if (m_Bodies.find(entity) == m_Bodies.end())
                CreateBody(scene, entity);
        }

        RemoveDestroyedBodies(scene);

        m_Accumulator += std::clamp(deltaTime, 0.0f, 0.25f);

        while (m_Accumulator >= m_FixedTimeStep)
        {
            b2World_Step(m_World, m_FixedTimeStep, m_SubStepCount);
            m_Accumulator -= m_FixedTimeStep;
        }

        SynchronizeTransforms(scene);
    }

    void Physics2D::DestroyBody(entt::entity entity)
    {
        const auto it = m_Bodies.find(entity);
        if (it == m_Bodies.end())
            return;

        b2DestroyBody(it->second);
        m_Bodies.erase(it);
    }

    void Physics2D::CreateBody(Scene& scene, entt::entity entity)
    {
        if (!m_Running ||
            !scene.Registry().valid(entity) ||
            m_Bodies.contains(entity) ||
            !scene.HasComponent<Component::Transform2d>(entity) ||
            !scene.HasComponent<Component::Rigidbody2D>(entity))
        {
            return;
        }

        const auto& transform =
            scene.GetComponent<Component::Transform2d>(entity);

        const auto& rigidbody =
            scene.GetComponent<Component::Rigidbody2D>(entity);

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = ToBox2DType(static_cast<int>(rigidbody.type));
        bodyDef.position = {transform.position.x, transform.position.y};
        bodyDef.rotation = b2MakeRot(glm::radians(transform.rotation));
        bodyDef.linearDamping = std::max(0.0f, rigidbody.linearDamping);
        bodyDef.angularDamping = std::max(0.0f, rigidbody.angularDamping);
        bodyDef.gravityScale = rigidbody.gravityScale;
        bodyDef.fixedRotation = rigidbody.fixedRotation;
        bodyDef.isBullet = rigidbody.bullet;
        bodyDef.isEnabled = rigidbody.enabled;

        const b2BodyId body = b2CreateBody(m_World, &bodyDef);
        m_Bodies.emplace(entity, body);

        if (!scene.HasComponent<Component::BoxCollider2D>(entity))
            return;

        const auto& collider =
            scene.GetComponent<Component::BoxCollider2D>(entity);

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = std::max(0.0f, collider.density);
        shapeDef.material.friction = std::max(0.0f, collider.friction);
        shapeDef.material.restitution = std::clamp(collider.restitution, 0.0f, 1.0f);
        shapeDef.isSensor = collider.sensor;
        shapeDef.enableContactEvents = true;
        shapeDef.enableSensorEvents = collider.sensor;

        const float halfWidth = std::max(
            0.001f,
            std::abs(collider.size.x * transform.scale.x) * 0.5f);

        const float halfHeight = std::max(
            0.001f,
            std::abs(collider.size.y * transform.scale.y) * 0.5f);

        const b2Polygon box = b2MakeOffsetBox(
            halfWidth,
            halfHeight,
            {collider.offset.x, collider.offset.y},
            b2MakeRot(0.0f));

        b2CreatePolygonShape(body, &shapeDef, &box);
    }

    void Physics2D::RemoveDestroyedBodies(Scene& scene)
    {
        std::vector<entt::entity> staleEntities;

        for (const auto& [entity, body] : m_Bodies)
        {
            (void)body;
            if (!scene.Registry().valid(entity) ||
                !scene.HasComponent<Component::Rigidbody2D>(entity))
            {
                staleEntities.push_back(entity);
            }
        }

        for (const auto entity : staleEntities)
            DestroyBody(entity);
    }

    void Physics2D::SynchronizeTransforms(Scene& scene)
    {
        for (const auto& [entity, body] : m_Bodies)
        {
            if (!scene.Registry().valid(entity) ||
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

            const b2Vec2 position = b2Body_GetPosition(body);
            const b2Rot rotation = b2Body_GetRotation(body);

            transform.position = {position.x, position.y};
            transform.rotation = glm::degrees(b2Rot_GetAngle(rotation));
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

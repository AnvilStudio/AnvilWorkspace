#pragma once

#include <box2d/box2d.h>
#include <vector>

namespace anv
{
    enum class CollisionEventPhase2D
    {
        Begin,
        End
    };

    enum class CollisionEventKind2D
    {
        Contact,
        Sensor
    };

    // Body IDs are only valid while their corresponding Box2D bodies exist.
    // Consumers should resolve these to stable scene entity IDs before dispatch.
    struct PhysicsCollisionEvent2D
    {
        b2BodyId bodyA = b2_nullBodyId;
        b2BodyId bodyB = b2_nullBodyId;
        CollisionEventPhase2D phase = CollisionEventPhase2D::Begin;
        CollisionEventKind2D kind = CollisionEventKind2D::Contact;
    };
}

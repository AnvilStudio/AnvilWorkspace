#pragma once

#include <box2d/box2d.h>
#include <vector>

namespace anv
{
    enum class CollisionEventPhase2D : uint8_t
    {
        Begin,
        End
    };

    enum class CollisionEventKind2D : uint8_t
    {
        Contact,
        Sensor
    };

    // Body IDs are only valid while their corresponding Box2D bodies exist.
    // Consumers should resolve these to stable scene entity IDs before dispatch.
    struct PhysicsCollisionEvent2D
    {
        b2ShapeId shapeA = b2_nullShapeId;
        b2ShapeId shapeB = b2_nullShapeId;
        CollisionEventPhase2D phase = CollisionEventPhase2D::Begin;
        CollisionEventKind2D kind = CollisionEventKind2D::Contact;
    };
}

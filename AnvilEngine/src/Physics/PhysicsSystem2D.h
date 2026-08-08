#pragma once

#include "Physics2D.h"

namespace anv
{
    // Transitional architecture alias.
    // PhysicsSystem2D is the scene-facing physics system name going forward.
    // The underlying Physics2D implementation will be split into
    // PhysicsSystem2D, PhysicsWorld2D, and PhysicsBody2D incrementally.
    using PhysicsSystem2D = Physics2D;
}

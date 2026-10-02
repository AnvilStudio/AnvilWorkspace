from __future__ import annotations

import _anvil
from dataclasses import dataclass
from enum import Enum, auto

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .entity import Entity


class RigidBody2D:
    _component_name = "RigidBody2D"

    def __init__(self, entity_id: str):
        self._entity_id = entity_id

    @property
    def linear_velocity(self):
        return _anvil._rigid_body_get_linear_velocity(self._entity_id)

    @linear_velocity.setter
    def linear_velocity(self, value):
        x, y = value
        _anvil._rigid_body_set_linear_velocity(
            self._entity_id,
            float(x),
            float(y)
        )

    def add_force(self, x: float, y: float):
        _anvil._rigid_body_add_force(
            self._entity_id,
            float(x),
            float(y)
        )

    def apply_impulse(self, x: float, y: float):
        _anvil._rigid_body_apply_impulse(
            self._entity_id,
            float(x),
            float(y)
        )

    # Temporary compatibility API. Prefer the entity-bound instance methods.
    @staticmethod
    def AddForce(entity_id: str, x: float, y: float):
        _anvil._rigid_body_add_force(
            entity_id,
            float(x),
            float(y)
        )

    @staticmethod
    def ApplyImpulse(entity_id: str, x: float, y: float):
        _anvil._rigid_body_apply_impulse(
            entity_id,
            float(x),
            float(y)
        )

class CollisionEventKind2D(Enum):
    CONTACT = auto()
    SENSOR = auto()


class CollisionEventPhase2D(Enum):
    BEGIN = auto()
    END = auto()


@dataclass
class PhysicsCollisionEvent2D:
    """Entity: Other entity Kind: Either a Hit, Collision, or Sensor Phase: Begin, End. 
    """
    entity: Entity
    kind: CollisionEventKind2D
    phase: CollisionEventPhase2D
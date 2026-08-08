import _anvil


class RigidBody2D:
    def __init__(self, entity_id: str):
        self._entity_id = entity_id

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

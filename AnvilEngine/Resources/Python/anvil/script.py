from _anvil import (
    _get_position,
    _set_position,
    _get_rotation,
    _set_rotation,
)

from .physics import RigidBody2D


class Script:
    def __init__(self):
        self.entity_id = ""

    @property
    def position(self):
        return _get_position(self.entity_id)

    @position.setter
    def position(self, value):
        x, y = value
        _set_position(
            self.entity_id,
            float(x),
            float(y)
        )

    @property
    def rotation(self):
        return _get_rotation(self.entity_id)

    @rotation.setter
    def rotation(self, value):
        _set_rotation(
            self.entity_id,
            float(value)
        )

    @property
    def rigid_body(self):
        return RigidBody2D(self.entity_id)

    def on_create(self):
        pass

    def on_update(self, delta_time: float):
        pass

    def on_destroy(self):
        pass

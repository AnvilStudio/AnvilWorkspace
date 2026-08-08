import _anvil

from .transform import Transform2D
from .physics import RigidBody2D
from .collider import BoxCollider2D


class Script:
    def __init__(self):
        self.entity_id = ""

    def has_component(self, component_type) -> bool:
        component_name = getattr(component_type, "_component_name", None)
        if component_name is None:
            raise TypeError("component_type must be an Anvil component type")

        return bool(
            _anvil._has_component(
                self.entity_id,
                component_name
            )
        )

    def get_component(self, component_type):
        component_name = getattr(component_type, "_component_name", None)
        if component_name is None:
            raise TypeError("component_type must be an Anvil component type")

        if not self.has_component(component_type):
            raise KeyError(
                f"Entity does not have component {component_name}"
            )

        return component_type(self.entity_id)

    @property
    def transform(self):
        return self.get_component(Transform2D)

    @property
    def rigid_body(self):
        return self.get_component(RigidBody2D)

    @property
    def box_collider(self):
        return self.get_component(BoxCollider2D)

    # Compatibility aliases. Prefer self.transform.position/rotation.
    @property
    def position(self):
        return self.transform.position

    @position.setter
    def position(self, value):
        self.transform.position = value

    @property
    def rotation(self):
        return self.transform.rotation

    @rotation.setter
    def rotation(self, value):
        self.transform.rotation = value

    def on_create(self):
        pass

    def on_update(self, delta_time: float):
        pass

    def on_destroy(self):
        pass

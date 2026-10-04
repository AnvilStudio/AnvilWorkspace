import _anvil

from .transform import Transform2D
from .physics import RigidBody2D
from .collider import BoxCollider2D


class Entity:
    def __init__(self, entity_id: str):
        self.entity_id = entity_id

    def has_component(self, component_type) -> bool:
        """
        Checks if an entity has a specific component
        
        Args:
            component_type: type of component
            
        Returns:
            Bool
            
        Example:
            if self.has_component(TagComponent): return True
        """
        
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
        """
        Retrieves a known component from an entity. will throw a key error if component not present.
        
        Args:
            component_type: type of component
        
        Returns:
            new Python wrapper from the components pointer.
            
        Examples:
            RigidBody = self.get_component(RigidBody2D)
        """
        
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


def find_entity(name: str):
    """
        Helper method that finds an entity by its name
        
        Args:
            name: string
            
        Returns: Python wrapper of the Entities pointer
            
        Examples:
            waypoint = find_entity("Waypoint1")
    """
    
    entity_id = _anvil._find_entity_by_name(str(name))
    if entity_id is None:
        return None

    return Entity(entity_id)

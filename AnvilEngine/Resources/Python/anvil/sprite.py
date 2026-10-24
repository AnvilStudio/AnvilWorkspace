import _anvil

class SpriteRenderer:
    _component_name = "SpriteRenderer"

    def __init__(self, entity_id: str):
        self._entity_id = entity_id

    @property
    def flip_x(self) -> bool:
        return bool(_anvil._sprite_get_flip(self._entity_id, "x"))

    @flip_x.setter
    def flip_x(self, value: bool):
        _anvil._sprite_set_flip(self._entity_id, "x", bool(value))

    @property
    def flip_y(self) -> bool:
        return bool(_anvil._sprite_get_flip(self._entity_id, "y"))

    @flip_y.setter
    def flip_y(self, value: bool):
        _anvil._sprite_set_flip(self._entity_id, "y", bool(value))

import _anvil


class Transform2D:
    _component_name = "Transform2D"

    def __init__(self, entity_id: str):
        self._entity_id = entity_id

    @property
    def position(self):
        return _anvil._get_position(self._entity_id)

    @position.setter
    def position(self, value):
        x, y = value
        _anvil._set_position(
            self._entity_id,
            float(x),
            float(y)
        )

    @property
    def rotation(self):
        return _anvil._get_rotation(self._entity_id)

    @rotation.setter
    def rotation(self, value):
        _anvil._set_rotation(
            self._entity_id,
            float(value)
        )

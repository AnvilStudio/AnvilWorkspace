import _anvil

class TagComponent:
    _component_name = "TagComponent"

    def __init__(self, entity_id: str):
        self._entity_id = entity_id

    @property
    def value(self) -> str:
        return _anvil._get_tag(self._entity_id)

    @value.setter
    def value(self, value: str):
        _anvil._set_tag(
            self._entity_id,
            str(value)
        )
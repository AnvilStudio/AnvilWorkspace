import _anvil


class Animator2D:
    """Control an entity's named sprite animation states."""

    _component_name = "Animator2D"

    def __init__(self, entity_id: str):
        self._entity_id = entity_id

    def play(self, state: str):
        """Play a named state. Repeated calls on an already playing state do not restart it."""
        _anvil._animator_play_state(self._entity_id, str(state))

    def pause(self):
        _anvil._animator_pause(self._entity_id)

    def stop(self):
        _anvil._animator_stop(self._entity_id)

    @property
    def current_state(self) -> str:
        return _anvil._animator_current_state(self._entity_id)

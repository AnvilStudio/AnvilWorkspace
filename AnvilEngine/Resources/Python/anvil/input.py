import _anvil
from .key import *

class Input:
    @staticmethod
    def is_key_pressed(key: Key | int) -> bool:
        """Return True while the given keyboard key is held."""
        return bool(_anvil._is_key_pressed(int(key)))

    @staticmethod
    def is_key_just_pressed(key: Key | int) -> bool:
        """Return True once when the given keyboard key transitions from released to pressed."""
        return bool(_anvil._is_key_just_pressed(int(key)))


input = Input()

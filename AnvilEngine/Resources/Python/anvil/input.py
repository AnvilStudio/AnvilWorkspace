import _anvil
from  .key import *

class Input:
    @staticmethod
    def is_key_pressed(key: Key | int) -> bool:
        """Return True while the given keyboard key is held."""
        return bool(_anvil._is_key_pressed(int(key)))


input = Input()
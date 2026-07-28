import _anvil

class Input:
    @staticmethod
    def is_key_pressed(key) -> bool:
        return _anvil._is_key_pressed(int(key))


input = Input()
from .entity import Entity


class Script(Entity):
    def __init__(self):
        super().__init__("")

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

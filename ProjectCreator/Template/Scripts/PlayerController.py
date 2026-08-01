import anvil


class PlayerController(anvil.Script):
    speed: float = 5.0

    def on_create(self):
        anvil.log("PlayerController created")

    def on_update(self, delta_time: float):
        pass

    def on_destroy(self):
        pass

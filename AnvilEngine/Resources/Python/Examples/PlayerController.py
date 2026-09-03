import anvil

class PlayerController(anvil.Script):
    speed: float = 5.0

    def on_update(self, delta_time):
        position = self.position

        if anvil.input.is_key_pressed(anvil.Key.A):
            position.x -= self.speed * delta_time

        if anvil.input.is_key_pressed(anvil.Key.D):
            position.x += self.speed * delta_time

        if anvil.input.is_key_pressed(anvil.Key.W):
            position.y += self.speed * delta_time

        if anvil.input.is_key_pressed(anvil.Key.S):
            position.y -= self.speed * delta_time

        if anvil.input.is_key_pressed(anvil.Key.SPACE):
            anvil.log("SPACE key is pressed")

        self.position = position
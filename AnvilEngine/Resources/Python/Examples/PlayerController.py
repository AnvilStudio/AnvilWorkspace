import anvil


class PlayerController(anvil.Script):
    speed: float = 5.0
    player_name: str = "Player"
    log_lifecycle: bool = True

    def on_create(self):
        if self.log_lifecycle:
            anvil.log(f"Created {self.player_name} ({self.entity_id})")

    def on_update(self, delta_time: float):
        x, y = self.position
        self.position = (x + self.speed * delta_time, y)

    def on_destroy(self):
        if self.log_lifecycle:
            anvil.log(f"Destroyed {self.player_name} ({self.entity_id})")

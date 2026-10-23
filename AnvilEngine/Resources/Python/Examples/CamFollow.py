import anvil
import anvil.math

class CameraController(anvil.Script):
    """
    Game camera smoothly follows the player
    
    Note:
        an entity named "Player" MUST be present in the scene!
    """
    
    follow_speed: float = 8.0
    offset_x: float = 0.0
    offset_y: float = 0.0

    def on_create(self):
        self.player = anvil.find_entity("Player")

        if self.player is None:
            anvil.error("CameraController: \"Player\" entity not found")
            return

        self.player_transform = self.player.transform
        self.camera_transform = self.transform

    def on_update(self, delta_time: float):
        if self.player is None:
            return

        player_x, player_y = self.player_transform.position
        camera_x, camera_y = self.camera_transform.position

        target_x = player_x + self.offset_x
        target_y = player_y + self.offset_y

        t = min(self.follow_speed * delta_time, 1.0)

        camera_x = anvil.math.lerp(camera_x, target_x, t)
        camera_y = anvil.math.lerp(camera_y, target_y, t)

        self.camera_transform.position = (
            camera_x,
            camera_y
        )


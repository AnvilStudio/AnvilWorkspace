import anvil
from anvil.tag import TagComponent
from anvil.physics import (PhysicsCollisionEvent2D, CollisionEventKind2D, CollisionEventPhase2D)

class PlayerController(anvil.Script):
    speed: float = 5.0

    def on_collision(self, event: PhysicsCollisionEvent2D):
        name = event.entity.get_component(TagComponent).value
        if event.kind == CollisionEventKind2D.CONTACT:
            if event.phase == CollisionEventPhase2D.BEGIN:
                anvil.log(f"Begin Contact with {name}")
            if event.phase == CollisionEventPhase2D.END:
                anvil.log(f"END Contact with {name}")
        if event.kind == CollisionEventKind2D.SENSOR:
            if event.phase == CollisionEventPhase2D.BEGIN:
                anvil.log(f"Begin Contact with {name}")
            if event.phase == CollisionEventPhase2D.END:
                anvil.log(f"End Contact with {name}")
                
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
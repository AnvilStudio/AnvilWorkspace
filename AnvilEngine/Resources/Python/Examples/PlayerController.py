import anvil
import anvil.math
from anvil.tag import TagComponent
from anvil.physics import (PhysicsCollisionEvent2D, CollisionEventKind2D, CollisionEventPhase2D)

class PlayerController(anvil.Script):
    """
    Basic player controller using RigidBody2D
    
    Note:
        Player entity MUST have a RigidBody2D component!
    """
    
    speed: float = 10
    jump_force: float = 2
    acceleration: float = 2

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

    def on_create(self):
        self.body = self.get_component(anvil.RigidBody2D)
        anvil.log("Player controller started")

    def on_update(self, delta_time: float):
        horizontal = 0.0

        if anvil.input.is_key_pressed(anvil.Key.A):
            horizontal -= 1.0
        if anvil.input.is_key_pressed(anvil.Key.D):
            horizontal += 1.0
        velocity_x, velocity_y = self.body.linear_velocity

        target_velocity_x = horizontal * self.speed
        
        velocity_x = anvil.math.lerp(
            velocity_x,
            target_velocity_x,
            min(self.acceleration * delta_time, 1.0)
        )

        self.body.linear_velocity = (
            velocity_x,
            velocity_y
        )
        

        if anvil.input.is_key_just_pressed(anvil.Key.SPACE):
            self.body.apply_impulse(
                0.0,
                self.jump_force
            )





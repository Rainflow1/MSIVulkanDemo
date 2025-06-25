from MSIVulkanDemo import Behaviour, TransformComponent, Vector3, ObjectRef
import math

class Rotate(Behaviour):

    def start(self):
        self.totalTime = 0
        pass

    def update(self, deltatime):
        
        self.totalTime += deltatime

        transform = self.getComponent(TransformComponent)

        transform.setRotation((0.0, 0.0, math.radians(45) * self.totalTime));

        pass

    pass
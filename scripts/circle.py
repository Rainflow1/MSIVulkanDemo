from MSIVulkanDemo import Behaviour, TransformComponent, Vector3, ObjectRef
import math

class Circle(Behaviour):

    def start(self):
        self.point = self.property(ObjectRef, None)
        self.offset = self.property(Vector3(0.0, 5.0, 0.0))
        self.totalTime = 0
        pass

    def update(self, deltatime):
        
        self.totalTime += deltatime

        transform = self.getComponent(TransformComponent)

        if self.point == None:
            otherPos = self.offset
        else:
            otherPos = self.point.getComponent(TransformComponent).getPosition() + self.offset

        transform.setPosition((otherPos.x + math.cos(-self.totalTime * 2.0) * 10, otherPos.y, otherPos.z + math.sin(-self.totalTime * 2.0) * 10));

        pass

    pass
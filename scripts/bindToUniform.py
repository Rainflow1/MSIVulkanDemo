from MSIVulkanDemo import Behaviour, TransformComponent, MaterialComponent, String, ObjectRef
import math

class BindToUniform(Behaviour):

    def start(self):
        self.uniformName = self.property(String, "ta")
        self.bindedObj = self.property(ObjectRef, None)
        pass

    def update(self, deltatime):

        if self.bindedObj != None and self.uniformName != "":

            transform = self.bindedObj.getComponent(TransformComponent)
            material = self.getComponent(MaterialComponent)

            if material.hasUniform(self.uniformName):
                print("tak")
                material.setUniform(self.uniformName, transform.getPosition())

        pass

    pass
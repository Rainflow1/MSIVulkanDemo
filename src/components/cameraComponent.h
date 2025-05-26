#pragma once

#include "../component.h"
#include "../input.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class CameraComponent : public Component{
private:
    

public:
    CameraComponent(){
        
    }

    void provideInput(float deltaTime, Input& input){ //TODO temp name
        auto& transform = getComponent<TransformComponent>();

        if(input.getKey("w")){
            transform.setPosition(transform.getPosition() + transform.getRotationVec() * deltaTime);
        }
        if(input.getKey("s")){
            transform.setPosition(transform.getPosition() - transform.getRotationVec() * deltaTime);
        }
        if(input.getKey("a")){
            transform.setPosition(transform.getPosition() - glm::normalize(glm::cross(transform.getRotationVec(), {0.0f, 1.0f, 0.0f})) * deltaTime);
        }
        if(input.getKey("d")){
            transform.setPosition(transform.getPosition() + glm::normalize(glm::cross(transform.getRotationVec(), {0.0f, 1.0f, 0.0f})) * deltaTime);
        }

        float sensitivity = 0.1f;
        glm::vec2 offset = input.getMousePositionOffset() * sensitivity;

        static float yaw = 0.0, pitch = 0.0;

        yaw   += offset.x;
        pitch += offset.y;

        if(pitch > 89.0f)
            pitch = 89.0f;
        if(pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        transform.setRotation(0, glm::normalize(direction));
    }
    
    glm::mat4x4 getView(){
        auto& cameraTransform = getComponent<TransformComponent>();

        return glm::lookAt(cameraTransform.getPosition(), cameraTransform.getPosition() + cameraTransform.getRotationVec(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

};



}
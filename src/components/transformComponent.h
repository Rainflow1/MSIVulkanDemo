#pragma once

#include "../component.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class TransformComponent : public Component{
private:
    glm::vec3 position, scale, rotationVec;
    float rotationAngle;

public:
    TransformComponent(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale): position(position), scale(scale), rotationVec(glm::vec3(0.0f, 1.0f, 0.0f)){

    }

    glm::vec3 getPosition(){
        return position;
    }

    void setPosition(glm::vec3 pos){
        position = pos;
    }

    glm::vec3 getScale(){
        return scale;
    }

    glm::vec3 getRotationVec(){
        return rotationVec;
    }

    float getRotationAngle(){
        return rotationAngle;
    }

    void setRotation(float angle, glm::vec3 rot){
        rotationVec = rot;
        rotationAngle = angle;
    }

};



}
#pragma once

#include "../component.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class TransformComponent : public Component{
private:
    glm::vec3 position, scale;

public:
    TransformComponent(glm::vec3 position): position(position){

    }

    glm::vec3 getPosition(){
        return position;
    }

};



}
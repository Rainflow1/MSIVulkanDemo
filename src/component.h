#pragma once

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

class GameObject;

class Component{
    friend GameObject;

public:
    Component(){

    };
    Component(Component& copy): id(copy.id), owner(copy.owner){};
    Component(Component&&) = delete;

private:
    std::weak_ptr<GameObject> owner;
    uint32_t id;

protected:
    template<typename T>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    getComponent(){
        return owner.lock()->getComponent<T>();
    }

public:


};



}

#include "components/renderComponent.h"
#include "components/modelComponent.h"
#include "components/transformComponent.h"
#include "components/materialComponent.h"
#include "components/cameraComponent.h"
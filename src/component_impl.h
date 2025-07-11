#pragma once

#include "component_decl.h"
#include "gameobject_decl.h"
#include "gameobject_impl.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

Component::Component(ComponentParams& params): owner(params.owner), resourceManager(params.resourceManager), gameobjectManager(params.gameobjectManager){

};

Component::Component(const Component& copy): id(copy.id), owner(copy.owner), resourceManager(resourceManager){};


template<class T>
typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
Component::getComponent(){

    if(!owner.expired()){
        auto ptr = owner.lock();
        return ptr->getComponent<T>();
    }else{
        throw std::runtime_error("Bad ptr");
    }
}


}


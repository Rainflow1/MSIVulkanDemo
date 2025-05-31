#pragma once

#include <entt/entity/registry.hpp>

#include "component.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

class GameObject : public std::enable_shared_from_this<GameObject>{

private:
    std::shared_ptr<entt::registry> entityRegistry;
    const entt::entity entityID;

public:
    GameObject(std::shared_ptr<entt::registry> entityRegistry): entityRegistry(entityRegistry), entityID(entityRegistry->create()){
        
    }

    ~GameObject(){
        if(entityRegistry && entityRegistry->valid(entityID))
            entityRegistry->destroy(entityID);
    }

    GameObject(GameObject& other): entityRegistry(other.entityRegistry), entityID(other.entityID){
        
    }

    GameObject(GameObject&& other): entityRegistry(other.entityRegistry), entityID(other.entityID){
        other.entityRegistry = nullptr;
    }

    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    addComponent(const Args&... args){
        T& component = entityRegistry->emplace<T>(entityID, args...);
        static_cast<Component&>(component).owner = shared_from_this();
        return component;
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    getComponent(){

        if(!entityRegistry->all_of<T>(entityID)){
            throw std::runtime_error(std::string("Entity does not have component: ") + typeid(T).name());
        }

        return entityRegistry->get<T>(entityID);
    }

    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value>::type
    removeComponent(){
        entityRegistry->remove<T>(entityID);
    }

};


template<class T>
typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
inline Component::getComponent(){

    if(!owner.expired()){
        auto ptr = owner.lock();
        return ptr->getComponent<T>();
    }else{
        throw std::runtime_error("Bad ptr");
    }
}

}

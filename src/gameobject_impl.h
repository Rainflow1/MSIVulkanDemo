#pragma once

#include "gameobject_decl.h"
#include "component_decl.h"
#include "component_impl.h"
#include "componentsInfo.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

GameObject::GameObject(std::shared_ptr<entt::registry> entityRegistry, std::shared_ptr<ResourceManager> resourceManager, GameobjectManagerI* gameobjectManager): entityRegistry(entityRegistry), entityID(entityRegistry->create()), resourceManager(resourceManager), gameobjectManager(gameobjectManager){
    
}

GameObject::~GameObject(){
    if(entityRegistry && entityRegistry->valid(entityID))
        entityRegistry->destroy(entityID);
}

GameObject::GameObject(GameObject& other): entityRegistry(other.entityRegistry), entityID(other.entityID){
    
}

GameObject::GameObject(GameObject&& other): entityRegistry(other.entityRegistry), entityID(other.entityID){
    other.entityRegistry = nullptr;
}


json GameObject::saveToJson(){
    json object;
    object["components"] = std::vector<json>();
    for(auto& component : getAllComponents()){
        object["components"].push_back({{"type", getComponentName(*component)}, {"data", component->saveToJson()}});
    }
    
    return object;
}

void GameObject::loadFromJson(json obj){

    for(auto& component : obj["components"]){
        auto& comp = addComponent(component["type"]);
        comp.loadFromJson(component["data"]);
    }
}

void GameObject::remove(){
    removed = true;
}

bool GameObject::isRemoved(){
    return removed;
}

template<typename T, typename... Args>
typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
GameObject::addComponent(const Args&... args){

    ComponentParams params;
    params.gameobjectManager = gameobjectManager;
    params.resourceManager = resourceManager;
    params.owner = shared_from_this();

    T& component = entityRegistry->emplace<T>(entityID, params, args...);

    return component;

/*
    if constexpr(std::is_constructible<T, std::shared_ptr<ResourceManager>, Args...>::value){
        T& component = entityRegistry->emplace<T>(entityID, resourceManager, args...);
        static_cast<Component&>(component).owner = shared_from_this();
        static_cast<Component&>(component).gameobjectManager = gameobjectManager;
        component.afterResourceManager();

        return component;
    }else{
        T& component = entityRegistry->emplace<T>(entityID, args...);
        static_cast<Component&>(component).owner = shared_from_this();
        static_cast<Component&>(component).gameobjectManager = gameobjectManager;
        static_cast<Component&>(component).resourceManager = resourceManager;
        component.afterResourceManager();

        return component;
    }  
*/      
}


using componentName = std::string;


componentName GameObject::getComponentName(Component& component){
    return getComponentName(component, AllComponents());
}

template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
typename std::enable_if<
    std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...) ,
    componentName
>::type
GameObject::getComponentName(Component& component, TYPELIST<H, T...> list){
    try{
        return typeid(dynamic_cast<H&>(component)).name();
    }catch(std::exception e){
        return getComponentName(component, TYPELIST<T...>());
    }
}

template<template<typename> typename TYPELIST, typename H>
typename std::enable_if<
    std::is_base_of<Component, H>::value, 
    componentName
>::type
GameObject::getComponentName(Component& component, TYPELIST<H> list){
    try{
        return typeid(dynamic_cast<H&>(component)).name();
    }catch(std::exception e){
        throw std::runtime_error("Bad component");
    }
}


Component& GameObject::addComponent(componentName name){
    return addComponent(name, AllComponents());
}

template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
typename std::enable_if<
    (std::is_constructible_v<H, ComponentParams&>) 
    && std::is_base_of<Component, H>::value 
    && (std::is_base_of<Component, T>::value && ...) 
    && (std::is_constructible_v<T, ComponentParams&> && ...),
    Component&
>::type
GameObject::addComponent(componentName name, TYPELIST<H, T...> list){
    if(typeid(H).name() == name && !entityRegistry->all_of<H>(entityID)){
        if constexpr(std::is_constructible<H, std::shared_ptr<ResourceManager>>::value){
            return static_cast<Component&>(addComponent<H>(resourceManager));
        }else{
            return static_cast<Component&>(addComponent<H>());
        }
    }else{
        return addComponent(name, TYPELIST<T...>());
    }
}

template<template<typename> typename TYPELIST, typename H>
typename std::enable_if<
    (std::is_constructible_v<H, ComponentParams&>) 
    && std::is_base_of<Component, H>::value, 
    Component&
>::type
GameObject::addComponent(componentName name, TYPELIST<H> list){
    if(typeid(H).name() == name && !entityRegistry->all_of<H>(entityID)){
        return static_cast<Component&>(addComponent<H>());
    }else{
        throw std::runtime_error("Component of name: " + name + " not found");
    }
}


template<typename T>
typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
GameObject::getComponent(){

    if(!entityRegistry->all_of<T>(entityID)){
        throw std::runtime_error(std::string("Entity does not have component: ") + typeid(T).name());
    }

    return entityRegistry->get<T>(entityID);
}

template<typename T>
typename std::enable_if<std::is_base_of<Component, T>::value, bool>::type
GameObject::hasComponent(){
    return entityRegistry->all_of<T>(entityID);
}


bool GameObject::hasComponent(componentName name){
    return GameObject::hasComponent(name, AllComponents());
}

template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
typename std::enable_if<std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...), bool>::type
GameObject::hasComponent(componentName name, TYPELIST<H, T...> list){
    if(typeid(H).name() == name && entityRegistry->all_of<H>(entityID)){
        return true;
    }else{
        return hasComponent(name, TYPELIST<T...>());
    }
}

template<template<typename> typename TYPELIST, typename H>
typename std::enable_if<std::is_base_of<Component, H>::value, bool>::type
GameObject::hasComponent(componentName name, TYPELIST<H> list){
    if(typeid(H).name() == name && entityRegistry->all_of<H>(entityID)){
        return true;
    }else{
        return false;
    }
}


template<typename T, typename... Args>
typename std::enable_if<std::is_base_of<Component, T>::value>::type
GameObject::removeComponent(){
    entityRegistry->remove<T>(entityID);
}


std::vector<Component*> GameObject::getAllComponents(){
    return getAllComponents(AllComponents());
}

template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
typename std::enable_if<std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...), std::vector<Component*>>::type
GameObject::getAllComponents(TYPELIST<H, T...> list){
    std::vector<Component*> vec;

    if(hasComponent<H>()){
        vec.push_back(static_cast<Component*>(&getComponent<H>()));
    }

    std::vector<Component*> tail = getAllComponents(TYPELIST<T...>());

    vec.insert(vec.end(), tail.begin(), tail.end());

    return vec;
}

template<template<typename> typename TYPELIST, typename H>
typename std::enable_if<std::is_base_of<Component, H>::value, std::vector<Component*>>::type
GameObject::getAllComponents(TYPELIST<H> list){
    std::vector<Component*> vec;
    if(hasComponent<H>()){
        vec.push_back(static_cast<Component*>(&getComponent<H>()));
    }
    return vec;
}


std::vector<componentName> GameObject::getAllComponentsNames(){
    return getAllComponentsNames(AllComponents());
}

template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
typename std::enable_if<std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...), std::vector<componentName>>::type
static GameObject::getAllComponentsNames(TYPELIST<H, T...> list){
    std::vector<std::string> vec;

    vec.push_back(typeid(H).name());

    std::vector<std::string> tail = getAllComponentsNames(TYPELIST<T...>());

    vec.insert(vec.end(), tail.begin(), tail.end());

    return vec;
}

template<template<typename> typename TYPELIST, typename H>
typename std::enable_if<std::is_base_of<Component, H>::value, std::vector<componentName>>::type
static GameObject::getAllComponentsNames(TYPELIST<H> list){
    std::vector<std::string> vec;
    vec.push_back(typeid(H).name());
    return vec;
}


}

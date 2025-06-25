#pragma once

#include <entt/entity/registry.hpp>

#include "json.h"
#include "resourceManager.h"
#include "gameobjectManagerI.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

class Component;

class GameObject : public std::enable_shared_from_this<GameObject>, JsonI{

private:
    std::shared_ptr<entt::registry> entityRegistry;
    const entt::entity entityID;

    std::shared_ptr<ResourceManager> resourceManager;

    GameobjectManagerI* gameobjectManager; // TODO ptr to shared

    bool removed = false;

public:
    inline GameObject(std::shared_ptr<entt::registry> entityRegistry, std::shared_ptr<ResourceManager> resourceManager, GameobjectManagerI* gameobjectManager);

    inline ~GameObject();

    inline GameObject(GameObject& other);

    inline GameObject(GameObject&& other);


    inline json saveToJson();

    inline void loadFromJson(json obj);

    inline void remove();

    inline bool isRemoved();

    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    inline addComponent(const Args&... args);


    using componentName = std::string;


    inline componentName getComponentName(Component& component);

    template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
    typename std::enable_if<
        std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...) ,
        componentName
    >::type
    inline getComponentName(Component& component, TYPELIST<H, T...> list);

    template<template<typename> typename TYPELIST, typename H>
    typename std::enable_if<
        std::is_base_of<Component, H>::value, 
        componentName
    >::type
    inline getComponentName(Component& component, TYPELIST<H> list);


    inline Component& addComponent(componentName name);

    template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
    typename std::enable_if<
        (std::is_constructible<H, std::shared_ptr<ResourceManager>>::value || std::is_default_constructible<H>::value) 
        && std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...) 
        && ( (std::is_constructible<T, std::shared_ptr<ResourceManager>>::value || std::is_default_constructible<T>::value) && ...),
        Component&
    >::type
    inline addComponent(componentName name, TYPELIST<H, T...> list);

    template<template<typename> typename TYPELIST, typename H>
    typename std::enable_if<
        (std::is_constructible<H, std::shared_ptr<ResourceManager>>::value || std::is_default_constructible<H>::value) 
        && std::is_base_of<Component, H>::value, 
        Component&
    >::type
    inline addComponent(componentName name, TYPELIST<H> list);


    template<typename T>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    inline getComponent();

    template<typename T>
    typename std::enable_if<std::is_base_of<Component, T>::value, bool>::type
    inline hasComponent();


    inline bool hasComponent(componentName name);

    template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
    typename std::enable_if<std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...), bool>::type
    inline hasComponent(componentName name, TYPELIST<H, T...> list);

    template<template<typename> typename TYPELIST, typename H>
    typename std::enable_if<std::is_base_of<Component, H>::value, bool>::type
    inline hasComponent(componentName name, TYPELIST<H> list);


    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value>::type
    inline removeComponent();

    
    inline std::vector<Component*> getAllComponents();

    template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
    typename std::enable_if<std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...), std::vector<Component*>>::type
    inline getAllComponents(TYPELIST<H, T...> list);

    template<template<typename> typename TYPELIST, typename H>
    typename std::enable_if<std::is_base_of<Component, H>::value, std::vector<Component*>>::type
    inline getAllComponents(TYPELIST<H> list);


    inline static std::vector<componentName> getAllComponentsNames();

    template<template<typename, typename...> typename TYPELIST, typename H, typename... T>
    typename std::enable_if<std::is_base_of<Component, H>::value && (std::is_base_of<Component, T>::value && ...), std::vector<componentName>>::type
    inline static getAllComponentsNames(TYPELIST<H, T...> list);

    template<template<typename> typename TYPELIST, typename H>
    typename std::enable_if<std::is_base_of<Component, H>::value, std::vector<componentName>>::type
    inline static getAllComponentsNames(TYPELIST<H> list);

};


}

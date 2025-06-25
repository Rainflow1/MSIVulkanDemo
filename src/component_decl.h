#pragma once

#include "resourceManager.h"
#include "json.h"
#include "gameobjectManagerI.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

class GameObject;

class Component : JsonI{
    friend GameObject;

protected:
    std::shared_ptr<ResourceManager> resourceManager;
    GameobjectManagerI* gameobjectManager;
    std::weak_ptr<GameObject> owner;

public:
    Component();

    Component(std::shared_ptr<ResourceManager> resourceManager);

    Component(Component& copy);
    Component(Component&&) = delete;

private:
    uint32_t id;

protected:
    template<class T>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    getComponent();

    virtual void afterResourceManager();

public:
    virtual void guiDisplayInspector() = 0;

private:

};


}
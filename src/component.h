#pragma once

#include "resourceManager.h"
#include "json.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

class GameObject;

class Component : JsonI{
    friend GameObject;

protected:
    std::shared_ptr<ResourceManager> resourceManager; 

public:
    Component(){

    };

    Component(std::shared_ptr<ResourceManager> resourceManager): resourceManager(resourceManager){

    };

    Component(Component& copy): id(copy.id), owner(copy.owner), resourceManager(resourceManager){};
    Component(Component&&) = delete;

private:
    std::weak_ptr<GameObject> owner;
    uint32_t id;

protected:
    template<class T>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    getComponent();

public:
    virtual void guiDisplayInspector() = 0;

private:


};



}

#include "components/renderComponent.h"
#include "components/modelComponent.h"
#include "components/transformComponent.h"
#include "components/materialComponent.h"
#include "components/cameraComponent.h"
#include "components/skyboxRendererComponent.h"
#include "components/behaviourComponent.h"

namespace MSIVulkanDemo{

template<typename ... components> struct componentlist{
    static_assert(
        (std::is_base_of_v<Component, components> && ...), 
        "All components must be derivied from Component class"
    );
    static_assert(
        ((std::is_constructible_v<components, std::shared_ptr<ResourceManager>> || std::is_constructible_v<components>) && ...), 
        "All components must be constructible with ptr to ResourceManager or default constructible"
    );
};
using AllComponents = componentlist<RenderComponent, ModelComponent, TransformComponent, MaterialComponent, CameraComponent, SkyboxRendererComponent, BehaviourComponent>;

}
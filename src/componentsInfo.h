#pragma once

#include "components/renderComponent.h"
#include "components/modelComponent.h"
#include "components/transformComponent.h"
#include "components/materialComponent.h"
#include "components/cameraComponent.h"
#include "components/skyboxRendererComponent.h"
#include "components/scriptComponent.h"

namespace MSIVulkanDemo{

template<typename ... components> struct componentlist{
    static_assert(
        (std::is_base_of_v<Component, components> && ...), 
        "All components must be derivied from Component class"
    );
    static_assert(
        ((std::is_constructible_v<components, ComponentParams&>) && ...), 
        "All components must be constructible with components params"
    );
};
using AllComponents = componentlist<RenderComponent, ModelComponent, TransformComponent, MaterialComponent, CameraComponent, SkyboxRendererComponent, ScriptComponent>;

}
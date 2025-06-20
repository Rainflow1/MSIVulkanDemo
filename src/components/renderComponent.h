#pragma once

#include "../component.h"
#include "../vulkan/vulkanUniform.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class RenderComponent : public Component{
private:
    

public: 
    RenderComponent(){

    }

    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Render")){
            
        }
    }


    json saveToJson(){
        json component;
        
        return component;
    }

    void loadFromJson(json obj){
        return;
    }


};



}
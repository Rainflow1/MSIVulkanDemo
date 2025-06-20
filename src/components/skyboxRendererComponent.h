#pragma once

#include "../component.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class SkyboxRendererComponent : public Component{
private:
    

public: 
    SkyboxRendererComponent(){

    }

    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("SkyboxRenderer")){
            
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
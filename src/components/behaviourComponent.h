#pragma once

#include "../component.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class Behaviour{

public:
    Behaviour(){

    }

    virtual void Start() = 0;
    virtual void Update(float) = 0;

};


class BehaviourComponent : public Component{
private:
    

public: 
    BehaviourComponent(){

    }

    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Behaviour")){
            
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
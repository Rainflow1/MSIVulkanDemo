#pragma once

#include "../component_decl.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class TransformComponent : public Component{
private:
    glm::vec3 position, scale, rotation;

public:
    TransformComponent(): position(glm::vec3(0.0f, 0.0f, 0.0f)), scale(glm::vec3(1.0f, 1.0f, 1.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)){

    }

    TransformComponent(std::shared_ptr<ResourceManager> resMgr): position(glm::vec3(0.0f, 0.0f, 0.0f)), scale(glm::vec3(1.0f, 1.0f, 1.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)){

    }

    TransformComponent(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale): position(position), scale(scale), rotation(glm::vec3(0.0f, 0.0f, 0.0f)){

    }

    TransformComponent(glm::vec3 position): position(position), scale(glm::vec3(1.0f, 1.0f, 1.0f)), rotation(glm::vec3(0.0f, 0.0f, 0.0f)){

    }

    TransformComponent(const TransformComponent& copy): position(copy.position), scale(copy.scale), rotation(copy.rotation){

    }

    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Transform")){
            ImGui::DragFloat3("position", glm::value_ptr(position), 0.1f);
            ImGui::DragFloat3("scale", glm::value_ptr(scale), 0.1f);
            ImGui::DragFloat3("rotation", glm::value_ptr(rotation), 0.1f);
        }
    }

    glm::mat4 getModel(){
        glm::mat4 translateMat = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotateMat = glm::rotate(glm::mat4(1.0f), rotation.x, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

        return translateMat * rotateMat * scaleMat;
    }

    glm::vec3 getPosition(){
        return position;
    }

    void setPosition(glm::vec3 pos){
        position = pos;
    }

    glm::vec3 getScale(){
        return scale;
    }

    void setScale(glm::vec3 scal){
        scale = scal;
    }

    glm::vec3 getRotation(){
        return rotation;
    }

    void setRotation(glm::vec3 rot){
        rotation = rot;
    }


    json saveToJson(){
        json component;
        
        component["position"] = std::vector<float>();

        for(uint32_t i = 0; i < position.length(); i++){
            component["position"].push_back(position[i]);
        }

        component["scale"] = std::vector<float>();

        for(uint32_t i = 0; i < scale.length(); i++){
            component["scale"].push_back(scale[i]);
        }

        component["rotation"] = std::vector<float>();

        for(uint32_t i = 0; i < rotation.length(); i++){
            component["rotation"].push_back(rotation[i]);
        }

        return component;
    }

    void loadFromJson(json component){

        auto pos = component["position"].get<std::vector<float>>();

        for(uint32_t i = 0; i < pos.size(); i++){
            position[i] = pos[i];
        }

        auto scal = component["scale"].get<std::vector<float>>();

        for(uint32_t i = 0; i < scal.size(); i++){
            scale[i] = scal[i];
        }

        auto rot = component["rotation"].get<std::vector<float>>();

        for(uint32_t i = 0; i < rot.size(); i++){
            rotation[i] = rot[i];
        }

        return;
    }


};



}
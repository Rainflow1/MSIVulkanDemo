#pragma once

#include <glm/glm.hpp>

#include <iostream>
#include <vector>
#include <map>

namespace MSIVulkanDemo{


class Input{
public:
    enum keyAction{
        None,
        Released,
        Pressed,
        Hold
    };

private:
    std::map<std::string, keyAction> keys;
    glm::vec2 mousePosition = glm::vec2(0, 0), mousePositionOffset = glm::vec2(0, 0);

public:
    Input(){

    }

    void setKey(std::pair<std::string, keyAction> key){
        if(keys.count(key.first)){
            keys[key.first] = key.second;
        }else{
            keys.insert(key);
        }
    }

    void update(){
        for(auto& [key, val] : keys){
            if(val == Released){
                keys[key] = None;
            }
            if(val == Pressed){
                keys[key] = Hold;
            }
        }
        mousePositionOffset = {0, 0};
    }

    void setMouse(glm::vec2 mousePos, glm::vec2 mouseOffset){
        mousePosition = mousePos;
        mousePositionOffset = mouseOffset;
    }

    bool getKey(std::string key){
        return keys.count(key) > 0 ? (keys[key] == Hold || keys[key] == Released || keys[key] == Pressed) : false;
    }

    bool getKeyDown(std::string key){
        return keys.count(key) > 0 ? keys[key] == Released : false;
    }

    bool getKeyUp(std::string key){
        return keys.count(key) > 0 ? keys[key] == Pressed : false;
    }

    bool getMouseButton(){
        return false;
    }

    bool getMouseButtonUp(){
        return false;
    }

    bool getMouseButtonDown(){
        return false;
    }

    glm::vec2 getMousePosition(){
        return mousePosition;
    }

    glm::vec2 getMousePositionOffset(){
        return mousePositionOffset;
    }

};


}
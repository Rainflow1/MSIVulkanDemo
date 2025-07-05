#pragma once

#include <iostream>
#include <vector>

namespace MSIVulkanDemo{

class GameObject;

class GameobjectManagerI{

public:
    virtual std::shared_ptr<GameObject> spawnGameObject(std::string objName) = 0;

    virtual std::shared_ptr<GameObject> getGameObject(std::string objName) = 0;

    virtual std::vector<std::pair<std::string, std::shared_ptr<GameObject>>> getAllGameObjects() = 0;

    virtual void removeGameObject(std::string objName) = 0;

    virtual std::string getGameObjectName(std::shared_ptr<GameObject> obj) = 0;
};


}
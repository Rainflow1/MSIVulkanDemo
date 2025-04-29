#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <type_traits>
#include <any>

namespace MSIVulkanDemo{


class Resource{
private:

protected:
    std::string path; // TODO change to id (path + classname/hash)

public:
    Resource(std::string path): path(path){

    }

    virtual ~Resource(){}

    std::string getPath(){
        return path;
    }

    virtual void loadDependency(std::any dependency){
        return;
    }
};


class ResourceManager{
private:
    std::vector<std::weak_ptr<Resource>> resources;

public:
    template<typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
    getResource(std::string path){
        for(auto res : resources){
            if(!res.expired() && res.lock()->getPath() == path){
                return std::dynamic_pointer_cast<T>(res.lock()); // TODO to map
            }
        }
        std::shared_ptr<T> resPtr = std::make_shared<T>(path);
        for(auto res : resources){
            if(res.expired()){
                res = resPtr;
                return resPtr;
            }
        }
        resources.push_back(resPtr);
        return resPtr;
    }

    template<typename T, typename D>
    typename std::enable_if<std::is_base_of<Resource, T>::value>::type
    addDependency(D& dependency){ // TODO add multiple dependency, addDependency at any time
        for(auto res : resources){
            auto resPtr = std::dynamic_pointer_cast<T>(res.lock());
            if(resPtr){
                resPtr->loadDependency(std::make_any<D>(dependency));
            }
        }
    }

};



}
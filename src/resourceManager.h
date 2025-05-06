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

    virtual void loadDependency(std::vector<std::any> dependencies){
        return;
    }
};


class ResourceManager{
private:
    std::map<std::string, std::weak_ptr<Resource>> resources;
    std::map<size_t, std::vector<std::any>> dependencies;

public:
    template<typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
    getResource(std::string path){

        if(resources.find(path) == resources.end()){
            std::shared_ptr<T> resPtr = createResource<T>(path);
            resources.insert({path, resPtr});
            return resPtr;
        }

        if(!resources[path].expired()){
            return std::dynamic_pointer_cast<T>(resources[path].lock());
        }else{
            std::shared_ptr<T> resPtr = createResource<T>(path);
            resources[path] = resPtr;
            return resPtr;
        }

    }

    template<typename T, typename D>
    typename std::enable_if<std::is_base_of<Resource, T>::value>::type
    addDependency(D& dependency){ // TODO add multiple dependency
        size_t id = typeid(T).hash_code();
        dependencies.insert({id, std::vector({std::make_any<D>(dependency)})});
    }

private:
    template<typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
    createResource(std::string path){
        std::shared_ptr<T> resPtr = std::make_shared<T>(path);

        size_t id = typeid(T).hash_code();

        if(dependencies.find(id) != dependencies.end()){
            resPtr->loadDependency(dependencies[id]);
        }

        return resPtr;
    }

};



}
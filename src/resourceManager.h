#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>
#include <numeric>
#include <type_traits>
#include <any>

namespace MSIVulkanDemo{

class ResourceManager;

class Resource : public std::enable_shared_from_this<Resource>{
    friend ResourceManager;

private:
    std::vector<std::string> paths; // TODO change to id (path + classname/hash)
    std::chrono::system_clock::duration dateModified; // TODO support for multiple files

public:
    Resource(){}

    virtual ~Resource(){}

    std::string getPath(){
        return paths[0];
    }

    std::vector<std::string> getPaths(){
        return paths;
    }

    void refresh(){
        
    }

    std::string getCombinedPath(){
        return combinePaths(paths);
    }

    static std::string combinePaths(std::vector<std::string> paths){
        return std::accumulate(paths.begin(), paths.end(), std::string(""), [](std::string ss, std::string s){
            return ss.empty() ? s : ss + ";" + s;
        });
    }

private:
    virtual void loadDependency(std::vector<std::any> dependencies){
        return;
    }

    virtual void load(std::string path){
        return;
    }

    void setPath(std::vector<std::string> paths){
        this->paths = paths;
        /* TODO
        if(path.find(";") != std::string::npos){
            std::string sub = path.substr(0, path.find(";"));
            dateModified = std::filesystem::last_write_time(sub).time_since_epoch();
        }else{
            dateModified = std::filesystem::last_write_time(path).time_since_epoch();
        }
        */
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

    template<typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
    getResource(std::vector<std::string> paths){

        std::string path = Resource::combinePaths(paths);

        if(resources.find(path) == resources.end()){
            std::shared_ptr<T> resPtr = createResource<T>(paths);
            resources.insert({path, resPtr});
            return resPtr;
        }

        if(!resources[path].expired()){
            return std::dynamic_pointer_cast<T>(resources[path].lock());
        }else{
            std::shared_ptr<T> resPtr = createResource<T>(paths);
            resources[path] = resPtr;
            return resPtr;
        }

    }

    template<typename T, typename D>
    typename std::enable_if<std::is_base_of<Resource, T>::value>::type
    addDependency(const D& dependency){ // TODO add multiple dependency
        size_t id = typeid(T).hash_code();
        dependencies.insert({id, std::vector({std::make_any<D>(dependency)})});
    }

    void updateResources(){

        for(auto& [path, ptr] : resources){
            if(ptr.expired()){
                continue;
            }

            std::chrono::system_clock::duration timestamp;
            if(path.find(";") != std::string::npos){
                /*
                std::string str = path;
                size_t pos = 0;
                while ((pos = str.find(";")) != std::string::npos) {
                    std::string sub = str.substr(0, pos);
                    str.erase(0, pos + 1);

                    std::cout << std::filesystem::last_write_time(sub).time_since_epoch() << std::endl;
                }
                */
                std::string sub = path.substr(0, path.find(";"));
                timestamp = std::filesystem::last_write_time(sub).time_since_epoch();

            }else{
                timestamp = std::filesystem::last_write_time(path).time_since_epoch();
            }

            if(ptr.lock()->dateModified < timestamp){
                ptr.lock()->dateModified = timestamp;
                
                // TODO
            }
        }

    }

private:
    template<typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
    createResource(std::string path){
        std::shared_ptr<T> resPtr = std::make_shared<T>(path);

        std::static_pointer_cast<Resource>(resPtr)->setPath({path});

        size_t id = typeid(T).hash_code();

        if(dependencies.find(id) != dependencies.end()){
            std::static_pointer_cast<Resource>(resPtr)->loadDependency(dependencies[id]);
        }

        return resPtr;
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<Resource, T>::value, std::shared_ptr<T>>::type
    createResource(std::vector<std::string> paths){
        std::shared_ptr<T> resPtr = std::make_shared<T>(paths);

        std::static_pointer_cast<Resource>(resPtr)->setPath(paths);

        size_t id = typeid(T).hash_code();

        if(dependencies.find(id) != dependencies.end()){
            std::static_pointer_cast<Resource>(resPtr)->loadDependency(dependencies[id]);
        }

        return resPtr;
    }

};



}

#include "./resources/texture.h"
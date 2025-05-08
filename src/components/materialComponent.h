#pragma once

#include "../component.h"
#include "../resources/shaderProgram.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class MaterialComponent : public Component, public VulkanDescriptorSetOwner{
private:
    std::shared_ptr<ShaderProgram> shaderProgram; 
    std::vector<std::shared_ptr<VulkanDescriptorSet>> descriptorSet;

    std::map<std::string, std::vector<float>> userUniforms;

public:
    MaterialComponent(std::shared_ptr<ShaderProgram> shaderProgram) : shaderProgram(shaderProgram){
        
    }

    ~MaterialComponent(){
        
    }

    std::shared_ptr<VulkanGraphicsPipeline> getGraphicsPipeline(){
        return shaderProgram->getGraphicsPipeline();
    }

    template<typename t>
    std::pair<size_t, t> uniform(std::string name, t val){
        
        return std::pair<size_t, t>(shaderProgram->getGraphicsPipeline()->getUniformData().getOffset(name), val);
    }

    template<typename t>
    typename std::enable_if<(sizeof(t)%sizeof(float) == 0)>::type
    setUniform(std::string name, t val){
        std::vector<float> arr(sizeof(t)/sizeof(float));
        
        for(int i = 0; i < sizeof(t)/sizeof(float); i++){
            arr[i] = reinterpret_cast<float*>(&val)[i];
        }

        if(userUniforms.find(name) != userUniforms.end()){
            userUniforms[name] = arr;
            return;
        }
        
        userUniforms.insert({name, arr});
    }

    std::vector<std::pair<size_t, std::vector<float>>> getUserUniforms(){
        std::vector<std::pair<size_t, std::vector<float>>> uniforms;

        for(auto const& [key, val] : userUniforms){
            uniforms.push_back(std::pair<size_t, std::vector<float>>(shaderProgram->getGraphicsPipeline()->getUniformData().getOffset(key), val));
        }

        return uniforms;
    }

    void setDescriptorSet(std::vector<std::shared_ptr<VulkanDescriptorSet>> sets){
        descriptorSet = sets;
    }

    std::vector<std::shared_ptr<VulkanDescriptorSet>> getDescriptorSet(){
        return descriptorSet;
    }

};



}
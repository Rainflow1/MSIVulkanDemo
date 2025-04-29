#pragma once

#include "../component.h"
#include "../resources/shaderProgram.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class MaterialComponent : public Component{
private:
    std::shared_ptr<ShaderProgram> shaderProgram; 
    std::vector<std::shared_ptr<VulkanDescriptorSet>> descriptorSet;

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

    void setDescriptorSet(std::vector<std::shared_ptr<VulkanDescriptorSet>> sets){
        descriptorSet = sets;
    }

    std::vector<std::shared_ptr<VulkanDescriptorSet>> getDescriptorSet(){
        return descriptorSet;
    }

};



}
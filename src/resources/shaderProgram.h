#pragma once

#include "../vulkan/vulkanCore.h"
#include "../resourceManager.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class ShaderProgram : public Resource{
private:
    std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline;

public:
    ShaderProgram(std::string path){

    }

    ~ShaderProgram(){}

    std::shared_ptr<VulkanGraphicsPipeline> getGraphicsPipeline(){
        return graphicsPipeline;
    }

private:

    void loadDependency(std::vector<std::any> dependencies){

        std::shared_ptr<VulkanRenderPass> renderPass;

        for(auto& dep : dependencies){
            if(dep.type() == typeid(std::shared_ptr<VulkanRenderPass>)){
                renderPass = std::any_cast<std::shared_ptr<VulkanRenderPass>>(dep);
            }
        }

        std::shared_ptr<VulkanShader> vertexShader = std::make_shared<VulkanShader>(renderPass->getDevice(), getPath(), ShaderType::Vertex);
        std::shared_ptr<VulkanShader> fragmentShader = std::make_shared<VulkanShader>(renderPass->getDevice(), getPath(), ShaderType::Fragment);

        graphicsPipeline = renderPass->createGraphicsPipeline({vertexShader, fragmentShader});

        return;
    }

};


}
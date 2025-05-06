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
    ShaderProgram(std::string path): Resource(path){

    }

    ~ShaderProgram(){}

    std::shared_ptr<VulkanGraphicsPipeline> getGraphicsPipeline(){
        return graphicsPipeline;
    }

    void loadDependency(std::vector<std::any> dependencies){
        std::shared_ptr<VulkanRenderPass> renderPass = std::any_cast<std::shared_ptr<VulkanRenderPass>>(dependencies[0]);

        std::shared_ptr<VulkanShader> vertexShader = std::make_shared<VulkanShader>(renderPass->getDevice(), path, ShaderType::Vertex);
        std::shared_ptr<VulkanShader> fragmentShader = std::make_shared<VulkanShader>(renderPass->getDevice(), path, ShaderType::Fragment);

        graphicsPipeline = renderPass->createGraphicsPipeline({vertexShader, fragmentShader});

        return;
    }

};


}
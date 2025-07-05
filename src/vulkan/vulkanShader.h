#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>
#include "spirv_reflect.h"

#include "interface/vulkanDeviceI.h"
#include "vulkanVertexData.h"
#include "vulkanUniform.h"

#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

namespace MSIVulkanDemo{

enum ShaderType{
    Vertex = shaderc_vertex_shader,
    Fragment = shaderc_fragment_shader
};

class VulkanShader{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkShaderModule shaderModule = nullptr;
    ShaderType type;

    bool isLoaded = false;

public:
    VulkanShader(std::shared_ptr<VulkanDeviceI> device, ShaderType shaderType): device(device), type(shaderType){

    }

    ~VulkanShader(){
        if(shaderModule){
            vkDestroyShaderModule(device->getDevice(), shaderModule, nullptr);
        }
    }

    operator VkShaderModule() const{
        return shaderModule;
    }

    ShaderType getType(){
        return type;
    }

    virtual VulkanVertexData getVertexData() = 0;

    virtual std::vector<VulkanUniformData::bindingBlock> getUniformData() = 0;

protected:

    void loadCode(const std::vector<uint32_t>& compiledCode){
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = compiledCode.size() * sizeof(uint32_t);
        createInfo.pCode = compiledCode.data();

        if (vkCreateShaderModule(device->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        isLoaded = true;
    }

};



}
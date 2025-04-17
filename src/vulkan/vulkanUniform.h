#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanDeviceI.h"
#include "interface/vulkanBufferI.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 

namespace MSIVulkanDemo{

class VulkanUniformLayout;

class VulkanUniformData{
private:
    std::vector<uint32_t> sizes;
    std::weak_ptr<VulkanUniformLayout> uniformLayout;

public:
    VulkanUniformData(std::initializer_list<std::pair<std::string, size_t>> attributes){

        for(auto pair : attributes){
            sizes.push_back(static_cast<uint32_t>(pair.second));
        }

    }

    ~VulkanUniformData(){
        
    }

    uint32_t getSize(){
        uint32_t sum = 0;

        for(auto size : sizes){
            sum += size;
        }

        return sum;
    }

    uint32_t getSize(int n){
        return sizes[n];
    }

    uint32_t getCount(){
        return static_cast<uint32_t>(sizes.size());
    }

    uint32_t getOffset(uint32_t n){
        uint32_t offset = 0;

        for(uint32_t i = 0; i < n; i++){
            offset += sizes[i];
        }

        return offset;
    }

    std::shared_ptr<VulkanUniformLayout> getUniformLayout(std::shared_ptr<VulkanDeviceI> device){
        return std::make_shared<VulkanUniformLayout>(device, sizes.size()); // TODO save to uniformLayout
    }

};

class VulkanDescriptorSet;

class VulkanDescriptorPool{
private:
    VkDescriptorPool descriptorPool = nullptr;


    std::shared_ptr<VulkanDeviceI> device;
    
    std::vector<std::weak_ptr<VulkanDescriptorSet>> sets;

public:
    VulkanDescriptorPool(std::shared_ptr<VulkanDeviceI> device): device(device){

        VkDescriptorPoolSize poolSize = {};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = 1; 

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1000; // TODO to count

        if (vkCreateDescriptorPool(*device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

    }

    ~VulkanDescriptorPool(){
        vkDestroyDescriptorPool(*device, descriptorPool, nullptr);
    }

    operator VkDescriptorPool() const{
        return descriptorPool;
    }

    VulkanDeviceI& getDevice(){
        return *device;
    }

    std::shared_ptr<VulkanDescriptorSet> getDescriptorSet( VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer){
        return std::make_shared<VulkanDescriptorSet>(this, uniformData.getUniformLayout(device), uniformData, uniformBuffer);
    }

};


class VulkanUniformLayout{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkDescriptorSetLayout descriptorSetLayout = nullptr;

public:
    VulkanUniformLayout(std::shared_ptr<VulkanDeviceI> device, size_t count): device(device){

        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for(int i = 0; i < count; i++){
            VkDescriptorSetLayoutBinding uboLayoutBinding = {};
            uboLayoutBinding.binding = i;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            uboLayoutBinding.pImmutableSamplers = nullptr;

            bindings.push_back(uboLayoutBinding);
        }

        
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

    }

    ~VulkanUniformLayout(){
        vkDestroyDescriptorSetLayout(*device, descriptorSetLayout, nullptr);
    }

    operator VkDescriptorSetLayout() const{
        return descriptorSetLayout;
    }

    VkDescriptorSetLayout* getLayoutPtr(){
        return &descriptorSetLayout;
    }
};


class VulkanDescriptorSet{
private:
    VulkanDescriptorPool* descriptorPool;
    std::shared_ptr<VulkanUniformLayout> layout;

    VkDescriptorSet descriptorSet = nullptr;

public:
    VulkanDescriptorSet(VulkanDescriptorPool* descriptorPool, std::shared_ptr<VulkanUniformLayout> layout, VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer): descriptorPool(descriptorPool), layout(layout){

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layout->getLayoutPtr();

        if (vkAllocateDescriptorSets(descriptorPool->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }

        writeDescriptorSet(uniformData, uniformBuffer);
    }

    ~VulkanDescriptorSet(){
        
    }

    operator VkDescriptorSet() const{
        return descriptorSet;
    }

private:

    void writeDescriptorSet(VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer){

        std::vector<VkDescriptorBufferInfo> bufferInfos;

        for(uint32_t i = 0; i < uniformData.getCount(); i++){
            VkDescriptorBufferInfo bufferInfo = {};
            bufferInfo.buffer = uniformBuffer;
            bufferInfo.offset = uniformData.getOffset(i);
            bufferInfo.range = uniformData.getSize(i);
            bufferInfos.push_back(bufferInfo);
        }

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;

        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferInfos.size());

        descriptorWrite.pBufferInfo = bufferInfos.data();
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(descriptorPool->getDevice(), 1, &descriptorWrite, 0, nullptr);
    }
};



}
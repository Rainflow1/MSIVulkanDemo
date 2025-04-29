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
    std::map<std::string, std::pair<uint32_t, uint32_t>> attributes;
    std::vector<std::string> keys;
    size_t size = 0;
    size_t count = 0;
    size_t bindingCount = 0;
    std::weak_ptr<VulkanUniformLayout> uniformLayout;

public:
    VulkanUniformData(std::initializer_list<std::initializer_list<std::pair<std::string, size_t>>> attribs){

        for(auto block : attribs){
            for(auto pair : block){
                attributes.insert({pair.first, {bindingCount, pair.second}});
                count++;
                size += pair.second;
                keys.push_back(pair.first);
            }
            bindingCount++;
        }
    }

    VulkanUniformData(std::vector<std::vector<std::pair<std::string, size_t>>>& attribs){

        for(auto block : attribs){
            for(auto pair : block){
                attributes.insert({pair.first, {bindingCount, pair.second}});
                count++;
                size += pair.second;
                keys.push_back(pair.first);
            }
            bindingCount++;
        }
    }

    ~VulkanUniformData(){
        
    }

    size_t getSize(){
        return size;
    }

    size_t getSize(int n){
        std::string key = keys[n];
        return attributes[key].second;
    }

    std::vector<size_t> getSizes(){
        std::vector<size_t> sizes(bindingCount);

        for(const auto& [key, val] : attributes){
            sizes[val.first] += 1;
        }

        return sizes;
    }

    uint32_t getCount(){
        return static_cast<uint32_t>(count);
    }

    size_t getOffset(uint32_t n){
        size_t offset = 0;

        for(uint32_t i = 0; i < n; i++){
            offset += getSize(i);
        }

        return offset;
    }

    size_t getOffset(std::string key){

        if(!contains(key)){
            throw std::runtime_error("VulkanUniformData.getOffset() - Key doesnt exists");
        }

        return getOffset(std::find(keys.begin(), keys.end(), key) - keys.begin());
    }

    bool contains(std::string key){
        return std::find(keys.begin(), keys.end(), key) != keys.end();
    }

    uint32_t getIndex(uint32_t n, uint32_t k){
        std::vector<size_t> sizes = getSizes();

        uint32_t sum = 0;

        for(int i = 0; i < n; i++){
            sum + sizes[i];
        }
        return sum + k;
    }

    size_t getBindingCount(){
        return bindingCount;
    }

    

    std::shared_ptr<VulkanUniformLayout> getUniformLayout(std::shared_ptr<VulkanDeviceI> device){
        return std::make_shared<VulkanUniformLayout>(device, bindingCount); // TODO save to uniformLayout
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

    std::shared_ptr<VulkanDescriptorSet> getDescriptorSet( VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer, size_t offset){
        return std::make_shared<VulkanDescriptorSet>(this, uniformData.getUniformLayout(device), uniformData, uniformBuffer, offset);
    }

};



class VulkanDescriptorSet{
private:
    VulkanDescriptorPool* descriptorPool;
    std::shared_ptr<VulkanUniformLayout> layout;

    VkDescriptorSet descriptorSet = nullptr;
    size_t offset;

public:
    VulkanDescriptorSet(VulkanDescriptorPool* descriptorPool, std::shared_ptr<VulkanUniformLayout> layout, VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer, size_t offset): descriptorPool(descriptorPool), layout(layout), offset(offset){

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layout->getLayoutPtr();

        if (vkAllocateDescriptorSets(descriptorPool->getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor set!");
        }

        writeDescriptorSet(uniformData, uniformBuffer, offset);
    }

    ~VulkanDescriptorSet(){
        
    }

    size_t getOffset(){
        return offset;
    }

    operator VkDescriptorSet() const{
        return descriptorSet;
    }

private:
/*
    void writeDescriptorSet(VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer, size_t offset){

        std::vector<VkWriteDescriptorSet> descriptorWrites = {};
        std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfoss;

        auto sizes = uniformData.getSizes();

        for(uint32_t i = 0; i < sizes.size(); i++){

            std::vector<VkDescriptorBufferInfo> bufferInfos;

            for(uint32_t j = 0; j < sizes[i]; j++){
                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = uniformBuffer;
                bufferInfo.offset = offset + uniformData.getOffset(uniformData.getIndex(i, j));
                bufferInfo.range = uniformData.getSize(uniformData.getIndex(i, j));
                bufferInfos.push_back(bufferInfo);
            }

            bufferInfoss.push_back(bufferInfos);

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = i;
            descriptorWrite.dstArrayElement = 0;

            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = static_cast<uint32_t>(bufferInfoss[i].size());

            descriptorWrite.pBufferInfo = bufferInfoss[i].data();
            descriptorWrite.pImageInfo = nullptr; // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            descriptorWrites.push_back(descriptorWrite);
        }

        vkUpdateDescriptorSets(descriptorPool->getDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
*/

void writeDescriptorSet(VulkanUniformData& uniformData, VulkanBufferI& uniformBuffer, size_t offset){
    // TODO zweryfikuj poprawne działanie i nazwij temp
    std::vector<VkDescriptorBufferInfo> bufferInfos;

    auto sizes = uniformData.getSizes();
    int temp = 0;

    for(uint32_t j = 0; j < sizes.size(); j++){
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = offset + uniformData.getOffset(temp);
        bufferInfo.range = uniformData.getOffset(temp+sizes[j]) - uniformData.getOffset(temp);
        bufferInfos.push_back(bufferInfo);
        temp += sizes[j];
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
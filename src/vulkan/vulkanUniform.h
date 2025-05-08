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
    uint32_t maxBinding = 0;
    std::weak_ptr<VulkanUniformLayout> uniformLayout;

    const size_t minOffset = 64; // TODO get from device

public:
    VulkanUniformData(std::initializer_list<std::initializer_list<std::pair<std::string, size_t>>> attribs){ // FIXME

        for(auto block : attribs){
            for(auto pair : block){
                size_t sizeTemp = pair.second;
                if(sizeTemp < minOffset){
                    sizeTemp = minOffset;
                }

                attributes.insert({pair.first, {bindingCount, sizeTemp}});
                count++;
                size += sizeTemp;
                keys.push_back(pair.first);
                maxBinding = bindingCount;
            }
            bindingCount++;
        }
    }

    VulkanUniformData(std::map<uint32_t, std::vector<std::pair<std::string, size_t>>>& attribs){

        for(auto const& [binding, block] : attribs){
            for(auto pair : block){
                size_t sizeTemp = pair.second;
                if(sizeTemp < minOffset){
                //    sizeTemp = minOffset; TODO add min offset at the end
                }

                attributes.insert({pair.first, {binding, sizeTemp}});
                count++;
                size += sizeTemp;
                keys.push_back(pair.first);
                maxBinding = std::max(maxBinding, binding);
            }
            bindingCount++;
        }
    }

    VulkanUniformData(VulkanUniformData& copy): attributes(copy.attributes), keys(copy.keys), size(copy.size), count(copy.count), bindingCount(copy.bindingCount), uniformLayout(copy.uniformLayout), maxBinding(copy.maxBinding){

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
        std::vector<size_t> sizes(maxBinding+1);

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

    VulkanUniformData operator+(VulkanUniformData& other){
        VulkanUniformData newUniformData(*this);

        for(auto key : other.keys){
            if(!newUniformData.contains(key)){
                auto val = other.attributes[key];
                newUniformData.attributes.insert({key, {val.first, val.second}});
                newUniformData.keys.push_back(key);
            }
        }

        newUniformData.size += other.size;
        newUniformData.count += other.count;
        newUniformData.bindingCount = std::max(other.bindingCount, newUniformData.bindingCount);
        newUniformData.maxBinding = std::max(other.maxBinding, newUniformData.maxBinding);

        return newUniformData;
    }

    std::shared_ptr<VulkanUniformLayout> getUniformLayout(std::shared_ptr<VulkanDeviceI> device){
        return std::make_shared<VulkanUniformLayout>(device, *this); // TODO save to uniformLayout
    }

};


class VulkanUniformLayout{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkDescriptorSetLayout descriptorSetLayout = nullptr;

public:
    VulkanUniformLayout(std::shared_ptr<VulkanDeviceI> device, VulkanUniformData& uniformData): device(device){

        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for(int i = 0; i < uniformData.getSizes().size(); i++){
            VkDescriptorSetLayoutBinding uboLayoutBinding = {};
            uboLayoutBinding.binding = i;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
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
    VulkanDescriptorPool(std::shared_ptr<VulkanDeviceI> device, std::vector<std::pair<VkDescriptorType, uint32_t>> customSizes = {}): device(device){

        std::vector<VkDescriptorPoolSize> poolSizes;

        if(customSizes.size() > 0){
            for(auto customSize : customSizes){
                VkDescriptorPoolSize poolSize = {};
                poolSize.type = customSize.first;
                poolSize.descriptorCount = customSize.second;
                poolSizes.push_back(poolSize);
            }
        }else{
            VkDescriptorPoolSize poolSize = {};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = 1;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
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
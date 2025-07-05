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
public:
    typedef uint32_t binding_id;

    struct attribute{
        binding_id binding;
        std::string name;
        size_t size;
        uint32_t componentCount;
    };

    struct bindingBlock{
        binding_id binding;
        uint32_t set;
        std::vector<attribute> attribs;
        VkDescriptorType type;
        size_t size;
    };


private:
    std::map<std::string, attribute> attributes;
    std::map<binding_id, bindingBlock> blocks;

    size_t descriptorSize = 0;
    std::weak_ptr<VulkanUniformLayout> uniformLayout;

    const size_t minOffset;

public:
    VulkanUniformData(std::vector<bindingBlock> bindingBlocks, size_t minDeviceOffset): minOffset(minDeviceOffset){

        for(auto& blk : bindingBlocks){
            size_t blkSize = 0;
            for(auto& attrib : blk.attribs){
                attributes.insert({attrib.name, attrib});
                blkSize += attrib.size;
            }
            if(blkSize%minOffset != 0){
                blkSize += minOffset - blkSize%minOffset; // WARN test this
            }
            blk.size = blkSize;
            blocks.insert({blk.binding, blk});
            descriptorSize += blkSize;
        }
    }

    VulkanUniformData(VulkanUniformData& copy): attributes(copy.attributes), uniformLayout(copy.uniformLayout), blocks(copy.blocks), descriptorSize(copy.descriptorSize), minOffset(copy.minOffset){

    }

    ~VulkanUniformData(){
        
    }

    std::vector<bindingBlock> getBindings() const{
        std::vector<bindingBlock> bindings;

        for(const auto& [key, val] : blocks){
            bindings.push_back(val);
        }

        return bindings;
    }

    std::vector<bindingBlock> getBindings(VkDescriptorType type) const{
        std::vector<bindingBlock> bindings;

        for(const auto& [key, val] : blocks){
            if(val.type == type){
                bindings.push_back(val);
            }
        }

        return bindings;
    }

    std::vector<VkDescriptorType> getBindingsTypes() const{
        std::vector<VkDescriptorType> types;

        for(const auto& [key, val] : blocks){
            if(std::find(types.begin(), types.end(), val.type) == types.end()){
                types.push_back(val.type);
            }
        }

        return types;
    }

    std::vector<attribute> getAttributes() const{
        std::vector<attribute> attribs;

        for(const auto& [key, val] : attributes){
            attribs.push_back(val);
        }

        return attribs;
    }

    attribute getAttribute(std::string name) const{
        return attributes.at(name);
    }

    size_t getOffset(std::string name) const{

        auto& block = blocks.at(attributes.at(name).binding);

        size_t offset = getOffset(block.binding);

        for(const auto& val : block.attribs){
            if(val.name == name){
                return offset;
            }
            offset += val.size;
        }
        return offset;
    }

    size_t getOffset(binding_id binding) const{
        size_t offset = 0;

        for(const auto& [key, val] : blocks){
            if(key == binding){
                break;
            }
            offset += val.size;
        }

        if(offset%minOffset != 0){
            throw std::runtime_error(std::format("Uniform data blocks need to have minimum offset: {}", minOffset));
        }

        return offset;
    }

    size_t getSize() const{

        if(descriptorSize%minOffset!=0){
            throw std::runtime_error(std::format("Uniform data blocks need to have size multiple of minimum offset: {}", minOffset));
        }

        return descriptorSize;
    }

    size_t getSize(binding_id binding) const{
        size_t size = 0;

        for(const auto& attrib : blocks.at(binding).attribs){
            size += attrib.size;
        }
        
        return size;
    }

    bool contains(std::string key) const{
        return attributes.count(key) > 0;
    }

    VulkanUniformData operator+(VulkanUniformData& other){
        VulkanUniformData newUniformData(*this);

        newUniformData.blocks.insert(other.blocks.begin(), other.blocks.end()); // FIXME merge "blocks"
        newUniformData.attributes.insert(other.attributes.begin(), other.attributes.end());

        newUniformData.descriptorSize = 0;

        for(const auto& [key, blk] : newUniformData.blocks){
            newUniformData.descriptorSize += blk.size;
        }

        return newUniformData;
    }

    std::shared_ptr<VulkanUniformLayout> getUniformLayout(std::shared_ptr<VulkanDeviceI> device) const{
        return std::make_shared<VulkanUniformLayout>(device, *this); // TODO save to uniformLayout
    }

};


class VulkanUniformLayout{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkDescriptorSetLayout descriptorSetLayout = nullptr;

public:
    VulkanUniformLayout(std::shared_ptr<VulkanDeviceI> device, const VulkanUniformData& uniformData): device(device){

        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for(const auto& binding : uniformData.getBindings()){
            VkDescriptorSetLayoutBinding uboLayoutBinding = {};
            uboLayoutBinding.binding = binding.binding;
            uboLayoutBinding.descriptorType = binding.type;
            uboLayoutBinding.descriptorCount = 1; // TODO add array support
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
            uboLayoutBinding.pImmutableSamplers = nullptr;

            bindings.push_back(uboLayoutBinding);
        }

        
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (VkResult errCode = vkCreateDescriptorSetLayout(*device, &layoutInfo, nullptr, &descriptorSetLayout); errCode != VK_SUCCESS) {
            throw std::runtime_error(std::format("failed to create descriptor set layout: {}", static_cast<int>(errCode)));
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
            poolSize.descriptorCount = 3;
            poolSizes.push_back(poolSize);

            poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSize.descriptorCount = 3;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1000; // TODO to count

        if (VkResult errCode = vkCreateDescriptorPool(*device, &poolInfo, nullptr, &descriptorPool); errCode != VK_SUCCESS) {
            throw std::runtime_error(std::format("failed to create descriptor pool: {}", static_cast<int>(errCode)));
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

    std::shared_ptr<VulkanDescriptorSet> getDescriptorSet(const VulkanUniformData& uniformData, std::shared_ptr<VulkanBufferI> uniformBuffer, size_t offset){
        return std::make_shared<VulkanDescriptorSet>(this, uniformData.getUniformLayout(device), uniformBuffer, offset);
    }

};



class VulkanDescriptorSet{
private:
    VulkanDescriptorPool* descriptorPool;
    std::shared_ptr<VulkanUniformLayout> layout;

    std::shared_ptr<VulkanBufferI> uniformBuffer;
    std::map<std::string, std::pair<VkImageView, VkSampler>> textures;
    VkImageView defaultImageView;
    VkSampler defaultSampler;

    VkDescriptorSet descriptorSet = nullptr;
    size_t offset;

public:
    VulkanDescriptorSet(VulkanDescriptorPool* descriptorPool, std::shared_ptr<VulkanUniformLayout> layout, std::shared_ptr<VulkanBufferI> uniformBuffer, size_t offset): descriptorPool(descriptorPool), layout(layout), uniformBuffer(uniformBuffer), offset(offset){

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = *descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layout->getLayoutPtr();

        if (VkResult errCode = vkAllocateDescriptorSets(descriptorPool->getDevice(), &allocInfo, &descriptorSet); errCode != VK_SUCCESS) {
            throw std::runtime_error(std::format("failed to allocate descriptor set: {}", static_cast<int>(errCode)));
        }
    }

    ~VulkanDescriptorSet(){
        
    }

    size_t getOffset(){
        return offset;
    }

    operator VkDescriptorSet() const{
        return descriptorSet;
    }

    void setTexture(std::string name, VkImageView view, VkSampler sampler){
        if(textures.contains(name)){
            textures.at(name) = {view, sampler};
        }else{
            textures.insert({name, {view, sampler}});
        }
    }

    void writeDescriptorSet(const VulkanUniformData& uniformData){
        
        std::vector<VkWriteDescriptorSet> sets;

        std::vector<VkDescriptorBufferInfo> bufferInfos(uniformData.getBindings().size());
        std::vector<VkDescriptorImageInfo> imageInfos(uniformData.getBindings().size());

        uint32_t imgCounter = 0;
        uint32_t bufCounter = 0;

        for(const auto& binding : uniformData.getBindings()){

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSet;
            descriptorWrite.dstBinding = binding.binding;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = binding.type;

            auto& name = binding.attribs[0].name;
            VkDescriptorBufferInfo bufferInfo = {};
            VkDescriptorImageInfo imageInfo = {};

            switch (binding.type){
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                
                bufferInfo.buffer = *uniformBuffer;
                bufferInfo.offset = offset + uniformData.getOffset(binding.binding);
                bufferInfo.range = uniformData.getSize(binding.binding);
                bufferInfos[bufCounter] = bufferInfo;

                
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfos[bufCounter];

                bufCounter++;
                break;

            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:

                VkSampler sampler;
                VkImageView view;

                if(textures.contains(name)){
                    sampler = textures.at(name).second;
                    view = textures.at(name).first;
                }else{
                    sampler = defaultSampler;
                    view = defaultImageView;
                }

                
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = view;
                imageInfo.sampler = sampler;
                imageInfos[imgCounter] = imageInfo;

                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &imageInfos[imgCounter];

                imgCounter++;
                break;

            default:
                std::cout << "Unsupported descriptor type" << std::endl;
                continue;
            }

            sets.push_back(descriptorWrite);
        }


        vkUpdateDescriptorSets(descriptorPool->getDevice(), sets.size(), sets.data(), 0, nullptr);
    }

private:



};



}
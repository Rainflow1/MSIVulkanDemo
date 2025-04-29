#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <glm/vec3.hpp>

#include <iostream>
#include <vector>
#include <cstdint>
#include <type_traits>

#include "interface/vulkanDeviceI.h"
#include "interface/vulkanBufferI.h"
#include "interface/vulkanCommandBufferI.h"
#include "vulkanComponent.h"
#include "vulkanVertexData.h"
#include "vulkanUniform.h"

namespace MSIVulkanDemo{

class VulkanMemoryManager : public VulkanComponent<VulkanMemoryManager>{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VmaAllocator allocator = nullptr;

public:
    VulkanMemoryManager(std::shared_ptr<VulkanDeviceI> device): device(device){

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT ;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
        allocatorCreateInfo.physicalDevice = device->getPhysicalDevice();
        allocatorCreateInfo.device = *device;
        allocatorCreateInfo.instance = device->getPhysicalDevice().getInstance();
        
        if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
            throw std::runtime_error("failed to initialize VMA Allocator!");
        }
    }

    ~VulkanMemoryManager(){
        if(allocator){       
            vmaDestroyAllocator(allocator);
        }
    }

    operator VmaAllocator() const{
        return allocator;
    }

    std::shared_ptr<VulkanDeviceI> getDevice(){
        return device;
    }

    template<typename T, typename ...Args>
    typename std::enable_if<std::is_base_of<VulkanBufferI, T>::value, std::shared_ptr<T>>::type
    createBuffer(Args... args){

        return std::make_shared<T>(shared_from_this(), args...);
    }

};

class VulkanBuffer : public VulkanComponent<VulkanBuffer>, public VulkanBufferI{
protected:
    std::shared_ptr<VulkanMemoryManager> allocator;

    VkBuffer buffer = nullptr;  
    VmaAllocation allocation = nullptr;
    VkDeviceSize size = 0;
    VmaAllocationInfo allocationInfo = {};

public:
    VulkanBuffer(std::shared_ptr<VulkanMemoryManager> allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags property): allocator(allocator), size(size){

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        //bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = property;
        
        if (vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &buffer, &allocation, &allocationInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to create Buffer!");
        }
    }

    virtual ~VulkanBuffer(){
        if(buffer && allocation){
            vmaDestroyBuffer(*allocator, buffer, allocation);
        }
    }

    operator VkBuffer() const{
        return buffer;
    }

    VkDeviceSize getSize(){
        return size;
    }

    void copyBuffer(VulkanBuffer& srcBuffer, VulkanBuffer& dstBuffer, VkDeviceSize size){
        
        std::shared_ptr<VulkanCommandBufferI> commandBuffer = std::dynamic_pointer_cast<VulkanCommandBufferI>(allocator->getDevice()->createCommandBuffer());

        commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        commandBuffer->copyBuffer(srcBuffer, dstBuffer, size);

        commandBuffer->end();
        commandBuffer->submit();
    }
};

template<class V>
class VulkanStagingBuffer : public VulkanBuffer{
private:

public:
    VulkanStagingBuffer(std::shared_ptr<VulkanMemoryManager> allocator, V* data, size_t size): VulkanBuffer(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT){

        vmaCopyMemoryToAllocation(*allocator, static_cast<void*>(data), allocation, 0, size);

    }

    ~VulkanStagingBuffer(){}

    void bind(VulkanCommandBufferI& commandBuffer) const{
        return;
    }
};


class VulkanVertexBuffer : public VulkanBuffer{
private:
    uint32_t vertexCount;
    
public:
    VulkanVertexBuffer(std::shared_ptr<VulkanMemoryManager> allocator, VulkanVertexData& vertices): VulkanBuffer(allocator, vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT), vertexCount(vertices.getVertexCount()){

        VulkanStagingBuffer<float> stagingBuffer(allocator, vertices.data(), vertices.size());
        copyBuffer(stagingBuffer, *this, this->getSize());

    }

    ~VulkanVertexBuffer(){}

    void bind(VulkanCommandBufferI& commandBuffer) const{
        VkBuffer vertexBuffers[] = {buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    }

    uint32_t getVertexCount(){
        return vertexCount;
    }

};

class VulkanIndexBuffer : public VulkanBuffer{
private:
    uint32_t indexCount;
    
public:
    VulkanIndexBuffer(std::shared_ptr<VulkanMemoryManager> allocator, VulkanVertexData& vertices): VulkanBuffer(allocator, vertices.getIndicesSize(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT), indexCount(vertices.getIndicesCount()){

        if(!vertices.hasIndices()){
            throw std::runtime_error("Vertices had to have indices");
        }

        VulkanStagingBuffer<uint32_t> stagingBuffer(allocator, vertices.getIndicesData(), vertices.getIndicesSize());
        copyBuffer(stagingBuffer, *this, this->getSize());

    }

    ~VulkanIndexBuffer(){}

    void bind(VulkanCommandBufferI& commandBuffer) const{
        vkCmdBindIndexBuffer(commandBuffer, buffer, 0, VK_INDEX_TYPE_UINT32);
    }

    uint32_t getIndexCount(){
        return indexCount;
    }

};



class VulkanUniformBuffer : public VulkanBuffer{
    private:
        std::shared_ptr<VulkanDescriptorPool> descriptorPool;

        std::vector<std::shared_ptr<VulkanDescriptorSet>> descriptorSet;
        std::shared_ptr<VulkanDescriptorSet> bindedDescriptorSet;
        size_t bufferSize = 0;
        
    public:
        VulkanUniformBuffer(std::shared_ptr<VulkanMemoryManager> allocator, std::shared_ptr<VulkanDescriptorPool> descriptorPool, size_t size): VulkanBuffer(allocator, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT), descriptorPool(descriptorPool){
            
        }
    
        ~VulkanUniformBuffer(){}
    
        void bind(VulkanCommandBufferI& commandBuffer) const{
            throw std::runtime_error("Bind uniform buffer by binding descriptor set");
        }

        void bindDescriptorSet(VulkanCommandBufferI& commandBuffer, std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline, std::vector<std::shared_ptr<VulkanDescriptorSet>> uniformSet){

            VkDescriptorSet sets[1];

            for(auto uniform : uniformSet){
                if(std::find(descriptorSet.begin(), descriptorSet.end(), uniform) != descriptorSet.end()){
                    bindedDescriptorSet = uniform;
                    sets[0] = {*uniform}; //TODO to map
                    break;
                }
            }

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline, 0, 1, sets, 0, nullptr);
        }

        std::shared_ptr<VulkanDescriptorSet> createDescriptorSet(VulkanUniformData& uniformData){
            auto set = descriptorPool->getDescriptorSet(uniformData, *this, bufferSize);
            descriptorSet.push_back(set);
            bufferSize += uniformData.getSize();

            return set;
        }

        template<typename T>
        void uploadData(size_t offset, T& val){
            
            VkMemoryPropertyFlags memPropFlags;
            vmaGetAllocationMemoryProperties(*allocator, allocation, &memPropFlags);

            if(memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT){

                vmaCopyMemoryToAllocation(*allocator, static_cast<void*>(&val), allocation, bindedDescriptorSet->getOffset() + offset, sizeof(T));
            
            }else{
                throw std::runtime_error("Nieeeee");
            }

        }
    
    };



}
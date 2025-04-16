#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <glm/vec3.hpp>

#include <iostream>
#include <vector>
#include <cstdint>

#include "interface/vulkanDeviceI.h"
#include "interface/vulkanBufferI.h"
#include "vulkanComponent.h"
#include "vulkanCommandBuffer.h"
#include "vulkanVertexData.h"

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

    VulkanDeviceI& getDevice(){
        return *device;
    }

};

class VulkanBuffer : public VulkanComponent<VulkanBuffer>, public VulkanBufferI{
protected:
    std::shared_ptr<VulkanMemoryManager> allocator;

    VkBuffer buffer = nullptr;  
    VmaAllocation allocation = nullptr;
    VkDeviceSize size = 0;

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
        
        if (vmaCreateBuffer(*allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
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
        
        std::shared_ptr<VulkanCommandBuffer> commandBuffer = allocator->getDevice().createCommandBuffer();

        commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT); // TODO make beggin end into class "CommandBufferRecording"

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
};


class VulkanVertexBuffer : public VulkanBuffer{
private:

    
public:
    VulkanVertexBuffer(std::shared_ptr<VulkanMemoryManager> allocator, VulkanVertexData& vertices): VulkanBuffer(allocator, vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT){

        VulkanStagingBuffer<float> stagingBuffer(allocator, vertices.data(), vertices.size());
        copyBuffer(stagingBuffer, *this, this->getSize());

    }

    ~VulkanVertexBuffer(){}

};

}
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
#include "interface/vulkanImageI.h"
#include "interface/vulkanCommandBufferI.h"
#include "vulkanComponent.h"
#include "vulkanVertexData.h"
#include "vulkanUniform.h"
#include "vulkanGraphicsPipeline.h"

namespace MSIVulkanDemo{

class VulkanImage;

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

    template<typename T, typename ...Args>
    typename std::enable_if<std::is_base_of<VulkanImageI, T>::value, std::shared_ptr<T>>::type
    createImage(Args... args){
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
    VulkanBuffer(std::shared_ptr<VulkanMemoryManager> allocator, VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags properties): allocator(allocator), size(size){

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        //bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = properties;
        
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



    class VulkanImageView;

    class VulkanImage: public VulkanImageI, public VulkanComponent<VulkanImage>{
    private:
        std::shared_ptr<VulkanMemoryManager> allocator;
        std::shared_ptr<VulkanDeviceI> device; // WARN Used only if swapChainImage = true
        
        std::pair<uint32_t, uint32_t> resolution;

        VkImage image = nullptr;
        VmaAllocation allocation = nullptr;
        VkDeviceSize size = 0; // TODO
        VmaAllocationInfo allocationInfo = {};
        VkFormat format;
        VkImageCreateInfo imageInfo = {};
        VmaAllocationCreateInfo allocInfo = {};

        bool isSwapChainImage = false;

        std::vector<std::weak_ptr<VulkanImageView>> imageViews;
    
    public:
        VulkanImage(std::shared_ptr<VulkanMemoryManager> allocator, std::pair<uint32_t, uint32_t> resolution, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VmaAllocationCreateFlags properties = 0): allocator(allocator), resolution(resolution), format(format){
            
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = resolution.first;
            imageInfo.extent.height = resolution.second;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0; 

            //VkMemoryRequirements memRequirements;
            //vkGetImageMemoryRequirements(*allocator->getDevice(), image, &memRequirements);
            
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = properties;
            //allocInfo.memoryTypeBits = memRequirements.memoryTypeBits;

            vmaCreateImage(*allocator, &imageInfo, &allocInfo, &image, &allocation, &allocationInfo);
        }

        VulkanImage(std::shared_ptr<VulkanDeviceI> device, VkImage image, VkFormat format): isSwapChainImage(true), image(image), device(device), format(format){

        }
    
        ~VulkanImage(){
            if(image && allocator){
                vmaDestroyImage(*allocator, image, allocation);
            }
        }

        operator VkImage() const{
            return image;
        }

        VkFormat getFormat(){
            return format;
        }

        std::shared_ptr<VulkanDeviceI> getDevice(){
            if(isSwapChainImage){
                return device;
            }else{
                return allocator->getDevice();
            }
        }

        void bind(){ //TODO virtual?
            vkBindImageMemory(*allocator->getDevice(), image, allocation->GetMemory(), 0);
        }

        template<typename... Args>
        std::shared_ptr<VulkanImageView> createImageView(Args ... args){
            std::shared_ptr<VulkanImageView> iv = std::make_shared<VulkanImageView>(shared_from_this(), args ...);
            for(auto imageView : imageViews){
                if(imageView.expired()){
                    imageView = iv;
                    return iv;
                }
            }
            imageViews.push_back(iv);
            return iv;
        }

        void transitionImageLayout(){} // TODO

        void resize(std::pair<uint32_t, uint32_t> resolution){
            if(image && allocator){
                vmaDestroyImage(*allocator, image, allocation);
            }

            imageInfo.extent.width = resolution.first;
            imageInfo.extent.height = resolution.first;

            vmaCreateImage(*allocator, &imageInfo, &allocInfo, &image, &allocation, &allocationInfo);
        }

    };



    class VulkanTexture: public VulkanImage{
        //TODO
    };



    class VulkanImageView{
    private:
        std::shared_ptr<VulkanImage> image;
    
        VkImageView imageView = nullptr;
        VkImageViewCreateInfo createInfo = {};
    
    public:
        VulkanImageView(std::shared_ptr<VulkanImage> image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT): image(image){
            
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = *image;
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = image->getFormat();
    
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    
            createInfo.subresourceRange.aspectMask = aspectFlags;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
    
            if (vkCreateImageView(*image->getDevice(), &createInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    
        ~VulkanImageView(){
            /*for(auto framebuffer : framebuffers){
                framebuffer.reset();
            }*/
    
            if(imageView){
                vkDestroyImageView(*image->getDevice(), imageView, nullptr);
            }
        }

        operator VkImageView() const{
            return imageView;
        }

        void resize(std::pair<uint32_t, uint32_t> resolution){
            if(imageView){
                vkDestroyImageView(*image->getDevice(), imageView, nullptr);
            }
            
            image->resize(resolution);
            createInfo.image = *image;

            if (vkCreateImageView(*image->getDevice(), &createInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    
    };



}
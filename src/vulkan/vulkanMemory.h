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
#include "vulkanImageData.h"

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

        std::shared_ptr<VulkanDescriptorSet> createDescriptorSet(const VulkanUniformData& uniformData){

            auto set = descriptorPool->getDescriptorSet(uniformData, shared_from_this(), bufferSize);
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

        void uploadData(size_t offset, std::vector<float> val){
            
            VkMemoryPropertyFlags memPropFlags;
            vmaGetAllocationMemoryProperties(*allocator, allocation, &memPropFlags);

            if(memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT){

                vmaCopyMemoryToAllocation(*allocator, static_cast<void*>(val.data()), allocation, bindedDescriptorSet->getOffset() + offset, val.size() * sizeof(float));
            
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
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

    bool isSwapChainImage = false;

    std::vector<std::weak_ptr<VulkanImageView>> imageViews;

public:
    struct constructParameters{
        uint32_t layers = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VmaAllocationCreateFlags properties = 0;
        VkImageCreateFlags flags = 0;
    };

    VulkanImage(std::shared_ptr<VulkanMemoryManager> allocator, std::pair<uint32_t, uint32_t> resolution, constructParameters params = constructParameters()): allocator(allocator), resolution(resolution), format(params.format){
        
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = resolution.first;
        imageInfo.extent.height = resolution.second;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = params.layers;
        imageInfo.format = format;
        imageInfo.tiling = params.tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = params.usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = params.flags;

        //VkMemoryRequirements memRequirements;
        //vkGetImageMemoryRequirements(*allocator->getDevice(), image, &memRequirements);
        
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = params.properties;
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

    void resize(std::pair<uint32_t, uint32_t> resolution){
        if(image && allocator){
            vmaDestroyImage(*allocator, image, allocation);
        }

        imageInfo.extent.width = resolution.first;
        imageInfo.extent.height = resolution.first;

        vmaCreateImage(*allocator, &imageInfo, &allocInfo, &image, &allocation, &allocationInfo);
    }

    void transitionImageLayout(VkImageLayout newLayout){
        std::shared_ptr<VulkanCommandBufferI> commandBuffer = std::dynamic_pointer_cast<VulkanCommandBufferI>(allocator->getDevice()->createCommandBuffer());
        commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = layout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = imageInfo.arrayLayers;


        VkPipelineStageFlags srcStage, dstStage; // TODO to external method
        
        if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        commandBuffer->setBarrier(barrier, srcStage, dstStage);

        commandBuffer->end();
        commandBuffer->submit();

        layout = newLayout;
    }

    void copyBufferToImage(VulkanBuffer& srcBuffer, VulkanImage& dstImage){
        std::shared_ptr<VulkanCommandBufferI> commandBuffer = std::dynamic_pointer_cast<VulkanCommandBufferI>(allocator->getDevice()->createCommandBuffer());
        commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = imageInfo.arrayLayers;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            resolution.first,
            resolution.second,
            1
        };

        commandBuffer->copyBufferToImage(srcBuffer, dstImage, region);

        commandBuffer->end();
        commandBuffer->submit();
    }

};



class VulkanTexture: public VulkanImage{

public:
    enum textureType{ //TODO
        Normal,
        Atlas,
        Cubemap
    };

private:
    uint32_t layers;
    textureType texType;

public:
    VulkanTexture(
        std::shared_ptr<VulkanMemoryManager> allocator,
        VulkanImageData& imageData,
        textureType type,
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VmaAllocationCreateFlags properties = 0
    ): VulkanImage(allocator, imageData.getResolution(), {.layers = imageData.getLayersNum(), .format = format, .tiling = tiling, .usage = usage, .properties = properties, .flags = (type == Cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : static_cast<VkImageCreateFlags>(0))}), texType(type){
        
        if(imageData.size() <= 0){
            throw std::runtime_error("Image had to have data");
        }

        VulkanStagingBuffer<uint8_t> stagingBuffer(allocator, imageData.data(), imageData.size());
        
        transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, *this);
        transitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    }

    ~VulkanTexture(){}

    textureType getType(){
        return texType;
    }

    uint32_t getLayersCount(){
        return 1; // TODO 
    }

};



class VulkanImageView{
private:
    std::shared_ptr<VulkanImage> image;

    VkImageView imageView = nullptr;
    VkImageViewCreateInfo createInfo = {};

public:
    VulkanImageView(std::shared_ptr<VulkanImage> image, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1): image(image){
        
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = *image;
        createInfo.viewType = viewType;
        createInfo.format = image->getFormat();

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = aspectFlags;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = layerCount;

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



class VulkanTextureView: public VulkanImageView{
    
public:
    VulkanTextureView(std::shared_ptr<VulkanTexture> texture): VulkanImageView(std::static_pointer_cast<VulkanImage>(texture), VK_IMAGE_ASPECT_COLOR_BIT, (texture->getType() == VulkanTexture::Cubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D), (texture->getType() == VulkanTexture::Cubemap ? 6 : 1)){
        
    }

    ~VulkanTextureView(){

    }

};



}
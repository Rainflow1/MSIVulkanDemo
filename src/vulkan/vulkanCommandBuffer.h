#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanDeviceI.h"
#include "interface/vulkanCommandBufferI.h"
#include "vulkanMemory.h"
#include "vulkanPhysicalDevice.h"
#include "vulkanGraphicsPipeline.h"
#include "vulkanFramebuffer.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 

namespace MSIVulkanDemo{

class VulkanCommandBuffer;

class VulkanCommandPool : public VulkanComponent<VulkanCommandPool>{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkCommandPool commandPool = nullptr;
    std::vector<std::weak_ptr<VulkanCommandBuffer>> commandBuffers;
    

public:
    VulkanCommandPool(std::shared_ptr<VulkanDeviceI> device): device(device){

        QueueFamilyIndices queueFamilyIndices = device->getPhysicalDevice().findQueueFamilies(device->getPhysicalDevice());

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(*device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    ~VulkanCommandPool(){
        if(commandPool){
            vkDestroyCommandPool(*device, commandPool, nullptr);
        }
    }

    std::shared_ptr<VulkanCommandBuffer> createCommandBuffer(){
        std::shared_ptr<VulkanCommandBuffer> cb = std::make_shared<VulkanCommandBuffer>(shared_from_this());
        for(auto commandbuffer : commandBuffers){
            if(commandbuffer.expired()){
                commandbuffer = cb;
                return cb;
            }
        }
        commandBuffers.push_back(cb);
        return cb;
    }

    operator VkCommandPool() const{
        return commandPool;
    }

    std::shared_ptr<VulkanDeviceI> getDevice(){
        return device;
    }
};

enum CommandBufferState{
    Initial,
    Recording,
    RecordingRenderPass,
    Executable,
    Pending,
    Invalid
};

class VulkanCommandBuffer : public VulkanComponent<VulkanCommandBuffer>, public VulkanCommandBufferI{
private:
    std::shared_ptr<VulkanCommandPool> commandPool;

    VkCommandBuffer commandBuffer = nullptr;

    CommandBufferState state = Initial;

    std::shared_ptr<VulkanFramebuffer> bindedFramebuffer = nullptr;
    std::shared_ptr<VulkanGraphicsPipeline> bindedGraphicsPipeline = nullptr;
    std::shared_ptr<VulkanUniformBuffer> uniformBuffer;

public:
    VulkanCommandBuffer(std::shared_ptr<VulkanCommandPool> commandPool): commandPool(commandPool){
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = *commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(*commandPool->getDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        uniformBuffer = commandPool->getDevice()->createMemoryManager()->createBuffer<VulkanUniformBuffer>(commandPool->getDevice()->createDescriptorPool(), 1024 * 1024);
    }

    ~VulkanCommandBuffer(){
        vkFreeCommandBuffers(*commandPool->getDevice(), *commandPool, 1, &commandBuffer);
    }

    operator VkCommandBuffer() const{
        return commandBuffer;
    }

    VulkanCommandBuffer& reset(){
        vkResetCommandBuffer(commandBuffer, 0);

        state = CommandBufferState::Initial;

        bindedGraphicsPipeline.reset();

        return *this;
    }

    VulkanCommandBuffer& begin(VkCommandBufferUsageFlags flags = 0){

        if(state != CommandBufferState::Initial){
            reset();
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        state = CommandBufferState::Recording;

        return *this;
    }

    VulkanCommandBuffer& beginRenderPass(std::shared_ptr<VulkanFramebuffer> framebuffer, VkCommandBufferUsageFlags flags = 0){
        
        if(state != CommandBufferState::Recording){
            if(state != CommandBufferState::Initial){ // TODO end previus renderpass if state = recording renderpass
                throw std::runtime_error("Command buffer wrong state: trying to end before begin");
            }
            begin(flags);
        }

        framebuffer->beginRenderPass(commandBuffer);

        bindedFramebuffer = framebuffer;

        state = CommandBufferState::RecordingRenderPass;

        return *this;
    }

    VulkanCommandBuffer& bind(VulkanBufferI& buffer){

        buffer.bind(*this);

        return *this;
    }

    VulkanCommandBuffer& bind(std::vector<std::shared_ptr<VulkanBufferI>> buffers){

        for(auto buffer : buffers){
            buffer->bind(*this);
        }

        return *this;
    }

    VulkanCommandBuffer& bind(std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline){
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *graphicsPipeline);

        VulkanGraphicsPipeline::ViewportStateInfo viewport(graphicsPipeline->getSwapChain());

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport.viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &viewport.scissor);

        bindedGraphicsPipeline = graphicsPipeline;

        return *this;
    }

    VulkanCommandBuffer& bind(std::vector<std::shared_ptr<VulkanDescriptorSet>> descriptorSet){

        if(descriptorSet.size() <= 0){
            throw std::runtime_error("Descriptor set should not be empty");
        }

        uniformBuffer->bindDescriptorSet(*this, bindedGraphicsPipeline, descriptorSet);

        return *this;
    }

    
    uint32_t getWidth(){
        return bindedFramebuffer->getResolution().first;
    }

    uint32_t getHeight(){
        return bindedFramebuffer->getResolution().second;
    }

    std::shared_ptr<VulkanDescriptorSet> createDescriptorSet(const VulkanUniformData& uniformData){
        return uniformBuffer->createDescriptorSet(uniformData);
    }


    template<typename t>
    VulkanCommandBuffer& setUniform(std::pair<size_t, t> val){

        if(!bindedGraphicsPipeline){
            throw std::runtime_error("Need to bind graphics pipeline first");
        }

        uniformBuffer->uploadData(val.first, val.second);

        return *this;
    }

    VulkanCommandBuffer& setUniform(std::vector<std::pair<size_t, std::vector<float>>> uniforms){

        for(auto& uniform : uniforms){
            setUniform(uniform);
        }

        return *this;
    }

    VulkanCommandBuffer& setUniform(std::pair<size_t, std::vector<float>> val){

        if(!bindedGraphicsPipeline){
            throw std::runtime_error("Need to bind graphics pipeline first");
        }

        uniformBuffer->uploadData(val.first, val.second);

        return *this;
    }

    VulkanCommandBuffer& draw(uint32_t vertexCount, uint32_t indexCount = 0){

        if(indexCount > 0){
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

            return *this;
        }

        vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

        return *this;
    }

    VulkanCommandBuffer& draw(std::pair<uint32_t, uint32_t> count){

        if(count.second > 0){
            vkCmdDrawIndexed(commandBuffer, count.second, 1, 0, 0, 0);

            return *this;
        }

        vkCmdDraw(commandBuffer, count.first, 1, 0, 0);

        return *this;
    }

    VulkanCommandBuffer& copyBuffer(VulkanBufferI& src, VulkanBufferI& dst, VkDeviceSize size){
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

        return *this;
    }

    VulkanCommandBuffer& copyBufferToImage(VulkanBufferI& src, VulkanImageI& dst, VkBufferImageCopy& region){
        vkCmdCopyBufferToImage(commandBuffer, src, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        return *this;
    }

    VulkanCommandBuffer& setBarrier(VkImageMemoryBarrier& barrier, VkPipelineStageFlags srcStage = 0, VkPipelineStageFlags dstStage = 0){ //TODO add default values
        


        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        return *this;
    };

    VulkanCommandBuffer& end(){

        if(state != CommandBufferState::Recording){
            if(state != CommandBufferState::RecordingRenderPass){
                throw std::runtime_error("Command buffer wrong state: trying to end before begin");
            }
            endRenderPass();
        }

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        state = CommandBufferState::Pending;

        return *this;
    }

    VulkanCommandBuffer& endRenderPass(){

        if(state != CommandBufferState::RecordingRenderPass){
            throw std::runtime_error("Command buffer wrong state: trying to end renderpass before begin renderpass");
        }

        vkCmdEndRenderPass(commandBuffer);
        bindedFramebuffer = nullptr;

        state = CommandBufferState::Recording;

        return *this;
    }

    VulkanCommandBuffer& submit(VulkanSemaphore& waitSemaphore, VulkanSemaphore& signalSemaphore, VulkanFence& fence){
        
        if(state != CommandBufferState::Pending){

            switch(state){
            case CommandBufferState::RecordingRenderPass:
                endRenderPass();

            case CommandBufferState::Recording:
                end();
                break;
            
            default:
                throw std::runtime_error("Command buffer wrong state: trying submit without begin");
                break;
            }
        }
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore waitSemaphores[] = {waitSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        
        VkSemaphore signalSemaphores[] = {signalSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(commandPool->getDevice()->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        state = CommandBufferState::Initial;

        return *this;
    }

    VulkanCommandBuffer& submit(){

        if(state != CommandBufferState::Pending){
            if(state == CommandBufferState::RecordingRenderPass){ // TODO switch next time
                endRenderPass();
            }else if(state == CommandBufferState::Recording){
                end();
            }else{
                throw std::runtime_error("Command buffer wrong state: trying submit without begin");
            }
        }

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (vkQueueSubmit(commandPool->getDevice()->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
        vkQueueWaitIdle(commandPool->getDevice()->getGraphicsQueue()); // TODO add optional bool

        state = CommandBufferState::Initial;

        return *this;
    }

private:

    

};

}
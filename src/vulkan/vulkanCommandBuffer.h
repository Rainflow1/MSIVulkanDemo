#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanDeviceI.h"
#include "interface/vulkanBufferI.h"
#include "vulkanPhysicalDevice.h"
#include "vulkanFramebuffer.h"
#include "vulkanGraphicsPipeline.h"

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

    std::shared_ptr<VulkanCommandBuffer> getCommandBuffer(){
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
    default,
    begin,
    beginRenderPass,
    ended
};

class VulkanCommandBuffer : public VulkanComponent<VulkanCommandBuffer>{
private:
    std::shared_ptr<VulkanCommandPool> commandPool;

    VkCommandBuffer commandBuffer = nullptr;

    CommandBufferState state = default;

    std::shared_ptr<VulkanGraphicsPipeline> bindedGraphicsPipeline = nullptr;

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

    }

    ~VulkanCommandBuffer(){
        vkFreeCommandBuffers(*commandPool->getDevice(), *commandPool, 1, &commandBuffer);
    }

    operator VkCommandBuffer() const{
        return commandBuffer;
    }

    VulkanCommandBuffer& reset(){
        vkResetCommandBuffer(commandBuffer, 0);

        state = CommandBufferState::default;

        bindedGraphicsPipeline.reset(); // TODO maybe add to end and submit too

        return *this;
    }

    VulkanCommandBuffer& begin(VkCommandBufferUsageFlags flags = 0){

        if(state != CommandBufferState::default){
            reset();
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        state = CommandBufferState::begin;

        return *this;
    }

    VulkanCommandBuffer& beginRenderPass(VulkanFramebuffer& framebuffer, VkCommandBufferUsageFlags flags = 0){
        
        if(state != CommandBufferState::default){
            reset();
        }
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = flags;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        framebuffer.beginRenderPass(commandBuffer);

        state = CommandBufferState::beginRenderPass;

        return *this;
    }

    VulkanCommandBuffer& bind(VulkanBufferI& buffer){ // TODO bind vertexbuffer

        buffer.bind(*this, bindedGraphicsPipeline);

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

    template<typename t>
    VulkanCommandBuffer& uniform(t val){

        if(!bindedGraphicsPipeline){
            throw std::runtime_error("Need to bind graphics pipeline first");
        }

        std::vector<VkDescriptorSet> sets;

        sets

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *bindedGraphicsPipeline, 0, sets.size(), sets.data(), 0, nullptr);

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

    VulkanCommandBuffer& copyBuffer(VulkanBufferI& src, VulkanBufferI& dst, VkDeviceSize size){
        VkBufferCopy copyRegion = {};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

        return *this;
    }

    VulkanCommandBuffer& end(){

        if(state != CommandBufferState::begin){
            throw std::runtime_error("Command buffer wrong state: trying to end before begin");
        }

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        state = CommandBufferState::ended;

        return *this;
    }

    VulkanCommandBuffer& endRenderPass(){

        if(state != CommandBufferState::beginRenderPass){
            throw std::runtime_error("Command buffer wrong state: trying to end renderpass before begin renderpass");
        }

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }

        state = CommandBufferState::ended;

        return *this;
    }

    VulkanCommandBuffer& submit(VulkanSemaphore& waitSemaphore, VulkanSemaphore& signalSemaphore, VulkanFence& fence){
        
        if(state != CommandBufferState::ended){
            if(state == CommandBufferState::beginRenderPass){ // TODO switch next time
                endRenderPass();
            }else if(state == CommandBufferState::begin){
                end();
            }else{
                throw std::runtime_error("Command buffer wrong state: trying submit without begin");
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

        state = CommandBufferState::default;

        return *this;
    }

    VulkanCommandBuffer& submit(){

        if(state != CommandBufferState::ended){
            if(state == CommandBufferState::beginRenderPass){ // TODO switch next time
                endRenderPass();
            }else if(state == CommandBufferState::begin){
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

        state = CommandBufferState::default;

        return *this;
    }

private:

    

};

}
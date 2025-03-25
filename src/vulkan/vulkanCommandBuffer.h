#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanDeviceI.h"
#include "vulkanPhysicalDevice.h"
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

    std::shared_ptr<VulkanCommandBuffer> getCommandBuffer(){
        std::shared_ptr<VulkanCommandBuffer> cb = std::make_shared<VulkanCommandBuffer>(shared_from_this());
        commandBuffers.push_back(cb); //TODO pool like framebuffers
        return cb;
    }

    operator VkCommandPool() const{
        return commandPool;
    }

    std::shared_ptr<VulkanDeviceI> getDevice(){
        return device;
    }
};

class VulkanCommandBuffer : public VulkanComponent<VulkanCommandBuffer>{
private:
    std::shared_ptr<VulkanCommandPool> commandPool;

    VkCommandBuffer commandBuffer = nullptr;


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

    }

    operator VkCommandBuffer() const{
        return commandBuffer;
    }

    void reset(){
        vkResetCommandBuffer(commandBuffer, 0);
    }

    void begin(VulkanFramebuffer& framebuffer){
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        framebuffer.beginRenderPass(commandBuffer);
    }

    void draw(){
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    void end(){
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void submit(VulkanSemaphore& waitSemaphore, VulkanSemaphore& signalSemaphore, VulkanFence& fence){
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {waitSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkSemaphore signalSemaphores[] = {signalSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(commandPool->getDevice()->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }
    }

private:

    

};

}
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanDeviceI.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 

namespace MSIVulkanDemo{

class VulkanSemaphore{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkSemaphore semaphore = nullptr;

public:
    VulkanSemaphore(std::shared_ptr<VulkanDeviceI> device): device(device){

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        if(vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &semaphore) != VK_SUCCESS){
            throw std::runtime_error("failed to create semaphore!");
        }
    }

    ~VulkanSemaphore(){
        if(semaphore){
            vkDestroySemaphore(*device, semaphore, nullptr);
        }
    }

    operator VkSemaphore() const{
        return semaphore;
    }
};

class VulkanFence{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkFence fence = nullptr;

public:
    VulkanFence(std::shared_ptr<VulkanDeviceI> device, bool initSignaled = false): device(device){ // TODO change initSignaled to function

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if(initSignaled)
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if(vkCreateFence(*device, &fenceInfo, nullptr, &fence) != VK_SUCCESS){
            throw std::runtime_error("failed to create fence!");
        }

    }

    ~VulkanFence(){
        if(fence){
            vkDestroyFence(*device, fence, nullptr);
        }
    }

    void waitFor(){
        vkWaitForFences(*device, 1, &fence, VK_TRUE, UINT64_MAX);
    }

    void reset(){
        vkResetFences(*device, 1, &fence);
    }

    operator VkFence() const{
        return fence;
    }

private:

};

}
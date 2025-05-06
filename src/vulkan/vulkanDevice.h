#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanDeviceI.h"
#include "vulkanPhysicalDevice.h"
#include "vulkanSwapChain.h"
#include "vulkanCommandBuffer.h"
#include "vulkanSync.h"
#include "vulkanComponent.h"
#include "vulkanMemory.h"
#include "vulkanUniform.h"

#include <iostream>
#include <set>

namespace MSIVulkanDemo{

class VulkanDevice : public VulkanComponent<VulkanDevice>, public VulkanDeviceI{
private:
    VkDevice device = nullptr;
    VkQueue graphicsQueue = nullptr;
    VkQueue presentQueue = nullptr;

    const std::vector<const char*> deviceExtensions;

    float queuePriority = 1.0f;

    std::shared_ptr<VulkanPhysicalDevice> physicalDevice;

    std::weak_ptr<VulkanSwapChain> swapChain;
    std::weak_ptr<VulkanCommandPool> commandPool;
    std::weak_ptr<VulkanMemoryManager> memoryManager;
    std::vector<std::weak_ptr<VulkanDescriptorPool>> descriptorPools;
    std::vector<std::weak_ptr<VulkanSemaphore>> semaphores;
    std::vector<std::weak_ptr<VulkanFence>> fences;

public:
    VulkanDevice(std::shared_ptr<VulkanPhysicalDevice> physicalDevice, const std::vector<const char*> deviceExtensions): physicalDevice(physicalDevice), deviceExtensions(deviceExtensions){

        QueueFamilyIndices indices = physicalDevice->findQueueFamilies(*physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        VkPhysicalDeviceFeatures deviceFeatures{};
        createInfo.pEnabledFeatures = &deviceFeatures;

        if (vkCreateDevice(*physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

       
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

    }

    ~VulkanDevice(){
        if(device != VK_NULL_HANDLE)
            vkDestroyDevice(device, nullptr);
    }

    std::shared_ptr<VulkanSwapChain> getSwapChain(){
        if(swapChain.expired()){
            auto sc = std::make_shared<VulkanSwapChain>(shared_from_this());
            swapChain = sc;
            return sc;
        }else{
            return swapChain.lock();
        }
    }

    VkDevice getDevice(){
        return device;
    }

    VulkanPhysicalDevice& getPhysicalDevice(){
        return *physicalDevice;
    }

    std::weak_ptr<VulkanCommandPool> getCommandPool(){
        return commandPool;
    }

    VkQueue getGraphicsQueue(){
        return graphicsQueue;
    }

    VkQueue getPresentQueue(){
        return presentQueue;
    }

    operator VkDevice() const {
        return device;
    }

    std::shared_ptr<VulkanCommandBuffer> createCommandBuffer(){
        if(commandPool.expired()){
            auto cp = std::make_shared<VulkanCommandPool>(shared_from_this());
            commandPool = cp;
            return cp->createCommandBuffer();
        }else{
            return commandPool.lock()->createCommandBuffer();
        }
    }

    std::shared_ptr<VulkanMemoryManager> createMemoryManager(){
        if(memoryManager.expired()){
            auto mm = std::make_shared<VulkanMemoryManager>(shared_from_this());
            memoryManager = mm;
            return mm;
        }else{
            return memoryManager.lock();
        }
    }

    std::shared_ptr<VulkanMemoryManager> getMemoryManager(){
        return createMemoryManager();
    }

    std::shared_ptr<VulkanSemaphore> createSemaphore(){ 
        auto sh = std::make_shared<VulkanSemaphore>(shared_from_this());

        semaphores.push_back(sh);

        return sh;
    }

    std::shared_ptr<VulkanFence> createFence(bool initSignaled = false){ 
        auto fence = std::make_shared<VulkanFence>(shared_from_this(), initSignaled);

        fences.push_back(fence);

        return fence;
    }

    std::shared_ptr<VulkanDescriptorPool> createDescriptorPool(std::vector<std::pair<VkDescriptorType, uint32_t>> customSizes = {}){
        std::shared_ptr<VulkanDescriptorPool> dp = std::make_shared<VulkanDescriptorPool>(shared_from_this(), customSizes);
        for(auto descriptorPool : descriptorPools){
            if(descriptorPool.expired()){
                descriptorPool = dp;
                return dp;
            }
        }
        descriptorPools.push_back(dp);
        return dp;
    }

    void waitForIdle(){
        vkDeviceWaitIdle(device);
    }

};

}
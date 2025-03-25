#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanDeviceI.h"
#include "vulkanPhysicalDevice.h"
#include "vulkanSwapChain.h"
#include "vulkanCommandBuffer.h"
#include "vulkanSync.h"
#include "vulkanComponent.h"

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

    std::shared_ptr<VulkanSwapChain> createSwapChain(){
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
            return cp->getCommandBuffer();
        }else{
            return commandPool.lock()->getCommandBuffer();
        }
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

    void waitForIdle(){
        vkDeviceWaitIdle(device);
    }

};

}
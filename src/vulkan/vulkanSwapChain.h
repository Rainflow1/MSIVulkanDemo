#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanDeviceI.h"
#include "vulkanMemory.h"
#include "vulkanGraphicsPipeline.h"
#include "vulkanSync.h"
#include "vulkanUniform.h"
#include "vulkanRenderPass.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm>
#include <functional> 

namespace MSIVulkanDemo{

class VulkanSwapChain : public VulkanComponent<VulkanSwapChain>, public VulkanSwapChainI{
private:
    std::shared_ptr<VulkanDeviceI> device;

    VkSwapchainKHR swapChain = nullptr;

    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<std::shared_ptr<VulkanImageView>> imageViews;

    std::vector<std::function<void(VulkanSwapChain&)>> swapChainRecreateCallbacks;

    uint32_t imageCount;
    uint32_t minImageCount;

public:
    VulkanSwapChain(std::shared_ptr<VulkanDeviceI> device): device(device){

        createSwapChain();

    }

    ~VulkanSwapChain(){
        if(swapChain){
            vkDestroySwapchainKHR(*device, swapChain, nullptr);
        }
    }

    VkFormat getImageFormat(){
        return swapChainImageFormat;
    }

    VkExtent2D getSwapChainExtent(){
        return swapChainExtent;
    }

    std::shared_ptr<VulkanDeviceI> getDevice(){
        return device;
    }


    uint32_t getNextImage(VulkanSemaphore& semaphore){
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(*device, swapChain, UINT64_MAX, semaphore, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return -1;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        return imageIndex;
    }
    

    void presentImage(VulkanSemaphore& signalSemaphore, uint32_t imageId){
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore signalSemaphores[] = {signalSemaphore};

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageId;
        presentInfo.pResults = nullptr;
        VkResult result = vkQueuePresentKHR(device->getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
    }

    void recreateSwapChain(){
        vkDeviceWaitIdle(*device);

        VkSwapchainKHR oldSwapChain = swapChain;
        std::vector<std::shared_ptr<VulkanImageView>> oldImageViews = imageViews;

        createSwapChain(oldSwapChain);

        for(auto image : oldImageViews){
            image.reset();
        }

        vkDestroySwapchainKHR(*device, oldSwapChain, nullptr);

        for(auto cb : swapChainRecreateCallbacks){
            cb(*this);
        }
    }

    void addSwapChainRecreateCallback(std::function<void(VulkanSwapChainI&)> cb){
        swapChainRecreateCallbacks.push_back(cb);
    }

    std::vector<VkImage>& getSwapChainImages(){
        return swapChainImages;
    }

    std::vector<std::shared_ptr<VulkanImageView>>& getSwapChainImageViews(){
        return imageViews;
    }

    uint32_t getMinImageCount(){
        return minImageCount;
    }

    uint32_t getImageCount(){
        return imageCount;
    }

private:

    void createSwapChain(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE){
        SwapChainSupportDetails swapChainSupport = device->getPhysicalDevice().querySwapChainSupport(device->getPhysicalDevice());

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);

        swapChainImageFormat = surfaceFormat.format;

        minImageCount = swapChainSupport.capabilities.minImageCount;
        imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            // TODO log that
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = device->getPhysicalDevice().getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = swapChainImageFormat;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapChainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = device->getPhysicalDevice().findQueueFamilies(device->getPhysicalDevice());
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = oldSwapChain;

        if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(*device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(*device, swapChain, &imageCount, swapChainImages.data());

        imageViews.clear();
        for(auto swapChainImage : swapChainImages){
            auto tempPtr = std::shared_ptr<VulkanImage>(new VulkanImage(device, swapChainImage, getImageFormat()));
            imageViews.push_back(tempPtr->createImageView());
        }

    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(device->getPhysicalDevice().getSurface(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

};

}
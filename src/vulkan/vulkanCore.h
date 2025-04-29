#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkanInstance.h"
#include "vulkanDebug.h"
#include "vulkanDevice.h"
#include "vulkanPhysicalDevice.h"
#include "vulkanSwapChain.h"
#include "vulkanFramebuffer.h"
#include "vulkanGraphicsPipeline.h"
#include "vulkanMemory.h"
#include "vulkanVertexData.h"
#include "interface/vulkanRendererI.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

namespace MSIVulkanDemo{

class Vulkan{ // TODO to vulkan context

private:
    std::shared_ptr<VulkanInstance> instance;

    const bool enableValidationLayers = true;
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> instanceExtensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
    };

    std::shared_ptr<VulkanSurface> surface;
    std::shared_ptr<VulkanPhysicalDevice> physicalDevice;
    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<VulkanSwapChain> swapChain;
    std::shared_ptr<VulkanRenderPass> renderPass;
    std::shared_ptr<VulkanMemoryManager> memory;
    std::shared_ptr<VulkanDescriptorPool> descriptorPool;

    std::vector<std::shared_ptr<VulkanCommandBuffer>> commandBuffers;

    std::vector<std::shared_ptr<VulkanSemaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<VulkanSemaphore>> renderFinishedSemaphores;
    std::vector<std::shared_ptr<VulkanFence>> inFlightFences;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint64_t currentFrame = 0;

public:
    Vulkan(GLFWwindow* window){

        instance = std::shared_ptr<VulkanInstance>(new VulkanInstance(instanceExtensions, deviceExtensions, enableValidationLayers, validationLayers));
        surface = instance->createSurface(window);
        physicalDevice = instance->createPhysicalDevice(surface);
        device = physicalDevice->createLogicDevice();
        swapChain = device->createSwapChain();
        renderPass = swapChain->createRenderPass();

        commandBuffers = {device->createCommandBuffer(), device->createCommandBuffer()};

        memory = device->createMemoryManager();

        imageAvailableSemaphores = {device->createSemaphore(), device->createSemaphore()};
        renderFinishedSemaphores = {device->createSemaphore(), device->createSemaphore()};
        inFlightFences = {device->createFence(true), device->createFence(true)};

    }

    ~Vulkan(){

        if(instance){
            instance.reset();
        }
    }

    std::shared_ptr<VulkanRenderPass> getRenderPass(){
        return renderPass;
    }

    std::shared_ptr<VulkanMemoryManager> getMemoryManager(){
        return memory;
    }

    std::vector<std::shared_ptr<VulkanDescriptorSet>> registerDescriptorSet(VulkanUniformData& uniformData){
        std::vector<std::shared_ptr<VulkanDescriptorSet>> sets;
        for(auto buffer : commandBuffers){
            sets.push_back(buffer->createDescriptorSet(uniformData));
        }
        return sets;
    }

    void drawFrame(VulkanRendererI& renderer){
        uint32_t frameIndex = currentFrame%MAX_FRAMES_IN_FLIGHT;

        inFlightFences[frameIndex]->reset();

        commandBuffers[frameIndex]->reset();

        uint32_t imageId = swapChain->getNextImage(*imageAvailableSemaphores[frameIndex]);

        if(imageId == -1){
            return;
        }

        auto frameBuffer = swapChain->getFramebuffer(imageId, renderPass);
        commandBuffers[frameIndex]->beginRenderPass(frameBuffer);
        renderer.render(*commandBuffers[frameIndex]);
        commandBuffers[frameIndex]->submit(*imageAvailableSemaphores[frameIndex], *renderFinishedSemaphores[frameIndex], *inFlightFences[frameIndex]);


        swapChain->presentImage(*renderFinishedSemaphores[frameIndex], imageId);
        inFlightFences[frameIndex]->waitFor();

        currentFrame++;
    }

    void windowResized(GLFWwindow* window){
        if(swapChain){
            swapChain->recreateSwapChain();
        }
    }

    void waitIdle(){
        device->waitForIdle();
    }

private:

    

};

}




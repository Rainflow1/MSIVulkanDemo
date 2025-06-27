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
#include "vulkanRenderGraph.h"
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
#ifdef DEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif

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
    std::shared_ptr<VulkanMemoryManager> memory;
    std::shared_ptr<VulkanDescriptorPool> descriptorPool;



    std::shared_ptr<VulkanRenderGraph> renderGraph;

    

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint64_t currentFrame = 0;

public:
    Vulkan(GLFWwindow* window){

        instance = std::shared_ptr<VulkanInstance>(new VulkanInstance(instanceExtensions, deviceExtensions, enableValidationLayers, validationLayers));
        surface = instance->createSurface(window);
        physicalDevice = instance->createPhysicalDevice(surface);
        device = physicalDevice->createLogicDevice();
        swapChain = device->getSwapChain();
        memory = device->createMemoryManager();
        renderGraph = std::make_shared<VulkanRenderGraph>(swapChain); // TODO better initialization

    }

    ~Vulkan(){

        if(instance){
            instance.reset();
        }
    }

    std::shared_ptr<VulkanInstance> getInstance(){
        return instance;
    }

    std::shared_ptr<VulkanPhysicalDevice> getPhysicalDevice(){
        return physicalDevice;
    }

    std::shared_ptr<VulkanDevice> getDevice(){
        return device;
    }

    std::shared_ptr<VulkanMemoryManager> getMemoryManager(){
        return memory;
    }

    void drawFrame(){
        uint64_t frameIndex = currentFrame%MAX_FRAMES_IN_FLIGHT;

        renderGraph->render(frameIndex);

        currentFrame++;
    }

    void loadRenderGraph(VulkanRenderGraphBuilderI& builder){
        
        builder.buildRenderGraph(renderGraph);

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




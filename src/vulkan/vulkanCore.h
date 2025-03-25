#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkanInstance.h"
#include "vulkanDebug.h"
#include "vulkanDevice.h"
#include "vulkanPhysicalDevice.h"
#include "vulkanSwapChain.h"
#include "vulkanFramebuffer.h"
#include "vulkanGraphicsPipeline.h"

#include <iostream>
#include <fstream>
#include <vector>

namespace MSIVulkanDemo{
    
class Vulkan{

private:
    std::shared_ptr<VulkanInstance> instance;

    const bool enableValidationLayers = true;
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    std::shared_ptr<VulkanSurface> surface;
    std::shared_ptr<VulkanPhysicalDevice> physicalDevice;
    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<VulkanSwapChain> swapChain;
    std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline;
    std::shared_ptr<VulkanCommandBuffer> commandBuffer;

    std::shared_ptr<VulkanSemaphore> imageAvailableSemaphore;
    std::shared_ptr<VulkanSemaphore> renderFinishedSemaphore;
    std::shared_ptr<VulkanFence> inFlightFence;

public:
    Vulkan(GLFWwindow* window){
        instance = std::shared_ptr<VulkanInstance>(new VulkanInstance(deviceExtensions, enableValidationLayers, validationLayers));
        surface = instance->createSurface(window);
        physicalDevice = instance->createPhysicalDevice(surface);
        device = physicalDevice->createLogicDevice();
        swapChain = device->createSwapChain();
        graphicsPipeline = swapChain->createGraphicsPipeline();
        commandBuffer = device->createCommandBuffer();

        imageAvailableSemaphore = device->createSemaphore();
        renderFinishedSemaphore = device->createSemaphore();
        inFlightFence = device->createFence(true);
    }

    ~Vulkan(){

        if(instance){
            instance.reset();
        }
    }

    void drawFrame(){
        inFlightFence->reset();

        commandBuffer->reset();

        uint32_t imageId = swapChain->getNextImage(*imageAvailableSemaphore);

        auto frameBuffer = swapChain->getFramebuffer(graphicsPipeline, imageId);
        commandBuffer->begin(*frameBuffer);
        graphicsPipeline->bind(*commandBuffer);
        commandBuffer->draw();
        commandBuffer->end();
        commandBuffer->submit(*imageAvailableSemaphore, *renderFinishedSemaphore, *inFlightFence);

        swapChain->presentImage(*renderFinishedSemaphore, imageId);
        inFlightFence->waitFor();
    }

    void waitIdle(){
        device->waitForIdle();
    }

private:

    

};

}




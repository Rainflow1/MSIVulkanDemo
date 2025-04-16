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
    std::shared_ptr<VulkanGraphicsPipeline> graphicsPipeline;
    std::shared_ptr<VulkanMemoryManager> memory;
    std::shared_ptr<VulkanVertexBuffer> vertexBuffer;

    std::vector<std::shared_ptr<VulkanCommandBuffer>> commandBuffers;

    std::vector<std::shared_ptr<VulkanSemaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<VulkanSemaphore>> renderFinishedSemaphores;
    std::vector<std::shared_ptr<VulkanFence>> inFlightFences;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint64_t currentFrame = 0;

public:
    Vulkan(GLFWwindow* window){

        VulkanVertexData tak({{"pos", VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2)}, {"col", VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)}});

        tak.append({0.0f, -0.7f, 1.0f, 1.0f, 0.0f});
        tak.append({0.5f, 0.5f, 0.0f, 1.0f, 0.0f});
        tak.append({-0.5f, 0.5f, 0.0f, 0.0f, 1.0f});

        instance = std::shared_ptr<VulkanInstance>(new VulkanInstance(instanceExtensions, deviceExtensions, enableValidationLayers, validationLayers));
        surface = instance->createSurface(window);
        physicalDevice = instance->createPhysicalDevice(surface);
        device = physicalDevice->createLogicDevice();
        swapChain = device->createSwapChain();
        graphicsPipeline = swapChain->createGraphicsPipeline(tak);

        commandBuffers = {device->createCommandBuffer(), device->createCommandBuffer()};

        memory = device->createMemoryManager();
        vertexBuffer = std::shared_ptr<VulkanVertexBuffer>(new VulkanVertexBuffer(memory, tak));

        imageAvailableSemaphores = {device->createSemaphore(), device->createSemaphore()};
        renderFinishedSemaphores = {device->createSemaphore(), device->createSemaphore()};
        inFlightFences = {device->createFence(true), device->createFence(true)};

    }

    ~Vulkan(){

        if(instance){
            instance.reset();
        }
    }

    void drawFrame(){

        uint32_t frameIndex = currentFrame%MAX_FRAMES_IN_FLIGHT;

        inFlightFences[frameIndex]->reset();

        commandBuffers[frameIndex]->reset();

        uint32_t imageId = swapChain->getNextImage(*imageAvailableSemaphores[frameIndex]);

        if(imageId == -1){
            return;
        }

        auto frameBuffer = swapChain->getFramebuffer(graphicsPipeline, imageId);
        commandBuffers[frameIndex]->beginRenderPass(*frameBuffer);
        graphicsPipeline->bind(*commandBuffers[frameIndex]); // TODO swap method dependency
        commandBuffers[frameIndex]->bind(*vertexBuffer);
        commandBuffers[frameIndex]->draw();
        commandBuffers[frameIndex]->endRenderPass();
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




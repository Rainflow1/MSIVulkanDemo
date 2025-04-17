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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>

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
    std::shared_ptr<VulkanIndexBuffer> indexBuffer;
    std::shared_ptr<VulkanDescriptorPool> descriptorPool;

    std::vector<std::shared_ptr<VulkanUniformBuffer>> uniformBuffers;

    std::vector<std::shared_ptr<VulkanCommandBuffer>> commandBuffers;

    std::vector<std::shared_ptr<VulkanSemaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<VulkanSemaphore>> renderFinishedSemaphores;
    std::vector<std::shared_ptr<VulkanFence>> inFlightFences;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint64_t currentFrame = 0;

public:
    Vulkan(GLFWwindow* window){

        VulkanVertexData tak({{"pos", VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2)}, {"col", VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)}});

        tak.append({
            {-0.5f, -0.5f, 1.0f, 0.0f, 0.0f},
            {0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
            {0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
            {-0.5f, 0.5f, 1.0f, 1.0f, 1.0f}
        });

        tak.addIndices({0, 1, 2, 2, 3, 0});

        VulkanUniformData nie({{"model", sizeof(glm::mat4)}, {"view", sizeof(glm::mat4)}, {"proj", sizeof(glm::mat4)}});

        instance = std::shared_ptr<VulkanInstance>(new VulkanInstance(instanceExtensions, deviceExtensions, enableValidationLayers, validationLayers));
        surface = instance->createSurface(window);
        physicalDevice = instance->createPhysicalDevice(surface);
        device = physicalDevice->createLogicDevice();
        swapChain = device->createSwapChain();
        graphicsPipeline = swapChain->createGraphicsPipeline(tak, nie);

        commandBuffers = {device->createCommandBuffer(), device->createCommandBuffer()};
        

        memory = device->createMemoryManager();
        vertexBuffer = std::shared_ptr<VulkanVertexBuffer>(new VulkanVertexBuffer(memory, tak));
        indexBuffer = std::shared_ptr<VulkanIndexBuffer>(new VulkanIndexBuffer(memory, tak));
        descriptorPool = device->createDescriptorPool();
        uniformBuffers = {memory->createBuffer<VulkanUniformBuffer>(descriptorPool, nie), memory->createBuffer<VulkanUniformBuffer>(descriptorPool, nie)};

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



        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), swapChain->getSwapChainExtent().width / (float) swapChain->getSwapChainExtent().height, 0.1f, 10.0f);
        proj[1][1] *= -1;

        uniformBuffers[frameIndex]->uploadData(0, model);
        uniformBuffers[frameIndex]->uploadData(1, view);
        uniformBuffers[frameIndex]->uploadData(2, proj); 

        auto frameBuffer = swapChain->getFramebuffer(graphicsPipeline, imageId);
        commandBuffers[frameIndex]->beginRenderPass(*frameBuffer)
        .bind(graphicsPipeline)
        .bind(*vertexBuffer)
        .bind(*indexBuffer)
        .bind(*uniformBuffers[frameIndex])
        //.uniform<glm::mat4>(model) //TODO alternatywnie do uniform buffer
        //.uniform<glm::mat4>(view)
        //.uniform<glm::mat4>(proj)
        .draw(vertexBuffer->getVertexCount(), indexBuffer->getIndexCount())
        .submit(*imageAvailableSemaphores[frameIndex], *renderFinishedSemaphores[frameIndex], *inFlightFences[frameIndex]);

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




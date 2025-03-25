#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkanDebug.h"
#include "vulkanComponent.h"

#include <iostream>
#include <vector>

namespace MSIVulkanDemo{

class VulkanSurface;
class VulkanPhysicalDevice;

class VulkanInstance : public VulkanComponent<VulkanInstance>{
private:
    VkInstance instance = nullptr;

    std::weak_ptr<VulkanPhysicalDevice> physicalDevice;
    std::weak_ptr<VulkanSurface> surface;

    bool enableValidationLayers;
    std::unique_ptr<VulkanDebugMessenger> debugMessenger;

    const std::vector<const char*> deviceExtensions;

public:
    VulkanInstance(const std::vector<const char*> deviceExtensions, bool enableValidationLayers, const std::vector<const char*> validationLayers): deviceExtensions(deviceExtensions), enableValidationLayers(enableValidationLayers){

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle"; // TODO add parameter
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        
        if(enableValidationLayers){
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            createInfo.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&VulkanDebugMessenger::getDebugMessangerCreateInfo());
            
        }else{
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }

        EnumerateVulkanExtensions();

        if (enableValidationLayers){
            debugMessenger = std::move(std::unique_ptr<VulkanDebugMessenger>(new VulkanDebugMessenger(instance, validationLayers)));
        }

    }

    ~VulkanInstance(){

        debugMessenger.reset();

        if(instance != nullptr){
            vkDestroyInstance(instance, nullptr);
        }
    }

    std::shared_ptr<VulkanPhysicalDevice> createPhysicalDevice(std::weak_ptr<VulkanSurface> surface){
        if(physicalDevice.expired()){
            auto pd = std::make_shared<VulkanPhysicalDevice>(shared_from_this(), surface.lock(), deviceExtensions);
            physicalDevice = pd;
            return pd;
        }
        return physicalDevice.lock();
    }

    std::shared_ptr<VulkanSurface> createSurface(GLFWwindow* window = nullptr){
        if(surface.expired()){
            if(!window){
                throw std::runtime_error("Need GLFWwindow to initialize surface!"); //TODO temp
            }
            auto sf = std::make_shared<VulkanSurface>(shared_from_this(), window);
            surface = sf;
            return sf;
        }
        return surface.lock();
    }

    operator VkInstance() const {
        return instance;
    }

    VkInstance getInstance(){
        return instance;
    }

private:
    void EnumerateVulkanExtensions(){
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "available extensions:\n";

        for (const auto &extension : extensions)
        {
            std::cout << '\t' << extension.extensionName << '\n';
        }
    }

    std::vector<const char*> getRequiredExtensions(){
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

};

}
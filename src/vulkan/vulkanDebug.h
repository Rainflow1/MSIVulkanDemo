#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>

namespace MSIVulkanDemo{
    
    class VulkanDebugMessenger{
    private:
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        const std::vector<const char*> validationLayers;

    public:
        VulkanDebugMessenger(VkInstance instance, const std::vector<const char*> validationLayers): instance(instance), validationLayers(validationLayers){
            if (!checkValidationLayerSupport()) {
                throw std::runtime_error("validation layers requested, but not available!");
            }

            VkDebugUtilsMessengerCreateInfoEXT createInfo = getDebugMessangerCreateInfo();
    
            if (VkResult errCode = CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger); errCode != VK_SUCCESS) {
                throw std::runtime_error(std::format("failed to set up debug messenger: {}", static_cast<int>(errCode)));
            }
        }

        ~VulkanDebugMessenger(){
            DestroyDebugUtilsMessengerEXT(nullptr);
        }

        static VkDebugUtilsMessengerCreateInfoEXT getDebugMessangerCreateInfo(){
            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;
            createInfo.pUserData = nullptr;

            return createInfo;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData){
    
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl << std::endl;
        
            return VK_FALSE;
        }

    private:
        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
            auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            } else {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        }

        void DestroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator) {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(instance, debugMessenger, pAllocator);
            }
        }

        bool checkValidationLayerSupport() {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        
            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
            for (const char* layerName : validationLayers) {
                bool layerFound = false;
            
                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }
            
                if (!layerFound) {
                    return false;
                }
            }
            
            return true;
        }
    };

}
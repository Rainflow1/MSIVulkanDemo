#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "vulkanInstance.h"
#include "vulkanComponent.h"

#include <iostream>
#include <vector>

namespace MSIVulkanDemo{

class VulkanSurface : public VulkanComponent<VulkanSurface>{
private:
    std::shared_ptr<VulkanInstance> instance;
    VkSurfaceKHR surface;
    GLFWwindow* window;

public:
    VulkanSurface(std::shared_ptr<VulkanInstance> instance, GLFWwindow* window): instance(instance), window(window){

        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window);
        createInfo.hinstance = GetModuleHandle(nullptr);

        if (VkResult errCode = vkCreateWin32SurfaceKHR(*instance, &createInfo, nullptr, &surface); errCode != VK_SUCCESS) {
            throw std::runtime_error(std::format("failed to create window surface: {}", static_cast<int>(errCode)));
        }

    }

    ~VulkanSurface(){
        if(surface){
            vkDestroySurfaceKHR(*instance, surface, nullptr);
        }
    }

    operator VkSurfaceKHR const(){
        return surface;
    }

    operator GLFWwindow* const(){
        return window;
    }

private:


};

}
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>

#include "vulkan/vulkanCore.h"

namespace MSIVulkanDemo{

class App{

private:
    GLFWwindow* window;
    const uint32_t WIDTH = 800, HEIGHT = 600;

    std::unique_ptr<Vulkan> vulkan;

public:
    App(){
        // TODO delete instance where not needed
    }

    void run(){

        initWindow();
        vulkan = std::unique_ptr<Vulkan>(new Vulkan(window));
        mainLoop();
        cleanup();

    }

private:

    void initWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    

    void mainLoop(){
        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            vulkan->drawFrame();

        }
        vulkan->waitIdle();
    }

    void cleanup(){
        glfwDestroyWindow(window);
        glfwTerminate();
    }

};

}
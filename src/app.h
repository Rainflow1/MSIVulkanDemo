#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>

#include "vulkan/vulkanCore.h"
#include "ImGuiInterface.h"
#include "scene.h"

namespace MSIVulkanDemo{

class App{

private:
    GLFWwindow* window;
    const uint32_t WIDTH = 800, HEIGHT = 600;

    std::unique_ptr<Vulkan> vulkan;
    bool windowResized = false;
    bool windowHidden = false;

    std::shared_ptr<ImGuiInterface> imgui;

    std::chrono::steady_clock::time_point previousTime = std::chrono::high_resolution_clock::now();
    const std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();

public:
    App(){

    }

    void run(){

        initWindow();
        vulkan = std::unique_ptr<Vulkan>(new Vulkan(window));
        imgui = std::shared_ptr<ImGuiInterface>(new ImGuiInterface(*vulkan, window));
        mainLoop();
        cleanup();

    }

private:

    void initWindow(){
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
        
        if(width <= 0 || height <= 0){
            app->windowHidden = true;
            return;
        }else{
            app->windowHidden = false;
        }
        
        app->windowResized = true;
    }

    void fpsCalc(std::chrono::steady_clock::time_point currentTime, std::chrono::steady_clock::time_point startTime, float deltaTime){
        float totalTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        
        static uint32_t frames = 0, totalFrames = 0;
        static float cumulativeTime = 0.0f;
        cumulativeTime += deltaTime;
        frames++;
        totalFrames++;
        
        if(cumulativeTime >= 1.0f){
            cumulativeTime = 0.0f;
            std::cout << frames << ", " << totalFrames/totalTime << ", " << deltaTime << std::endl;
            frames = 0;
        }
    }

    void mainLoop(){

        SimpleScene scene;
        scene.loadGui(imgui);
        
        vulkan->loadRenderGraph(scene);
        scene.loadScene(*vulkan);

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();
            scene.update(deltaTime);
            fpsCalc(currentTime, startTime, deltaTime);
            previousTime = currentTime;

            if(windowHidden){
                continue;
            }

            if(windowResized){
                vulkan->windowResized(window);
                windowResized = false;
            }

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

/*
TODO

 - Image allocation with vma
 

*/
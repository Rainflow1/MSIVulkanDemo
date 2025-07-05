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

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>

#include "vulkan/vulkanCore.h"
#include "ImGuiInterface.h"
#include "scene.h"
#include "input.h"
#include "fileDialog.h"

namespace MSIVulkanDemo{



class App{

private:
    GLFWwindow* window;
    const uint32_t WIDTH = 1920, HEIGHT = 1080;

    std::unique_ptr<Vulkan> vulkan;
    bool windowResized = false;
    bool windowHidden = false;

    bool guiMode = true;
    Input inputMap;

    std::shared_ptr<ImGuiInterface> imgui;

    std::chrono::steady_clock::time_point previousTime = std::chrono::high_resolution_clock::now();
    const std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    uint32_t FPS = 0;

    std::unique_ptr<Scene> scene;

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
        glfwSetKeyCallback(window, inputKeyCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);

        FileDialog::fileDialog(window);
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

    static void inputKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
        auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

        if(key == GLFW_KEY_TAB && action == GLFW_RELEASE){
            app->guiMode = !app->guiMode;

            if(app->guiMode){
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }else{
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }
        }

        if(!app->guiMode){

            const char* keyNameChar = glfwGetKeyName(key, scancode);

            std::string keyName;

            if(keyNameChar){
                //std::cout << keyNameChar << std::endl;
                keyName = std::string(keyNameChar);
            }else{
                switch(key){
                case GLFW_KEY_LEFT_SHIFT:
                    keyName = "shift"; // TODO more keys
                    break;
                
                default:
                    keyName = "~";
                    break;
                }
            }

            Input::keyAction act = Input::None;

            switch (action){
            case GLFW_RELEASE:
                act = Input::Released;
                break;
            case GLFW_PRESS:
                act = Input::Pressed;
                break;
            case GLFW_REPEAT:
                act = Input::Hold;
                break;
            default:
                break;
            }

            app->inputMap.setKey({keyName, act});
        }
    }

    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos){
        auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));

        static glm::vec2 lastMousePos = {xpos, ypos};

        if(!app->guiMode){

            float xoffset = xpos - lastMousePos.x;
            float yoffset = lastMousePos.y - ypos; 

            app->inputMap.setMouse({xpos, ypos}, {xoffset, yoffset});
        }

        lastMousePos = {xpos, ypos};
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
            FPS = frames;
            frames = 0;
        }
    }

    void loadScene(){
        scene = std::unique_ptr<Scene>(new SimpleScene());
        scene->loadGui(imgui);
        
        vulkan->loadRenderGraph(*scene);
        scene->loadScene(*vulkan);
    }

    void mainLoop(){

        loadScene();

        while(!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();
            
            scene->updateScene(deltaTime, inputMap);
            inputMap.update();
            
            fpsCalc(currentTime, startTime, deltaTime);
            previousTime = currentTime;

            if(windowHidden){
                continue;
            }

            if(windowResized){
                vulkan->windowResized(window);
                windowResized = false;
            }

            menuBar();

            vulkan->drawFrame();

        }
        vulkan->waitIdle();
    }

    void menuBar(){
        if(ImGui::BeginMainMenuBar()){

            if(guiMode){
                ImGui::Text(std::string("GUI Mode [TAB]").c_str());
            }else{
                ImGui::Text(std::string("Camera Mode [TAB]").c_str());
            }

            ImGui::Text((std::to_string(FPS) + " fps").c_str());

            if (ImGui::BeginMenu("Scene")){
                if (ImGui::MenuItem("Open scene", "Ctrl+O")){
                    std::string filePath = FileDialog::fileDialog().getPath();
                    if(!filePath.empty()){
                        std::ifstream inputFile(filePath);
                        scene->loadFromJson(json::parse(inputFile)["scene"]);
                    }
                }

                if (ImGui::MenuItem("Save scene", "Ctrl+S")){
                    std::string filePath = FileDialog::fileDialog().savePath("scene.json");
                    if(!filePath.empty()){
                        std::ofstream outputFile(filePath);
                        outputFile << std::setw(2) << json({{"scene", scene->saveToJson()}});
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void cleanup(){
        FileDialog::fileDialog().clear();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

};

}

/*
TODO

 -shadowmap
 -shadery
 -ladowanie scen
 -fixy
 

*/
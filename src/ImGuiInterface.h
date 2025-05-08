#pragma once

#include "vulkan/vulkanCore.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <iostream>
#include <vector>
#include <memory>
#include <type_traits>
#include <any>

namespace MSIVulkanDemo{


class ImGuiInterface : public VulkanRendererI{
private:
    std::shared_ptr<VulkanInstance> instance;
    std::shared_ptr<VulkanPhysicalDevice> physicalDevice;
    std::shared_ptr<VulkanDevice> device;
    std::shared_ptr<VulkanDescriptorPool> descriptorPool;

    bool hasVulkanInited = false;

public:
    ImGuiInterface(Vulkan& vulkan, GLFWwindow* window){

        instance = vulkan.getInstance();
        physicalDevice = vulkan.getPhysicalDevice();
        device = vulkan.getDevice();
        descriptorPool = device->createDescriptorPool({
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        });

        device->getSwapChain()->addSwapChainRecreateCallback([](VulkanSwapChainI& swapChain){
            ImGui_ImplVulkan_SetMinImageCount(swapChain.getMinImageCount());
            //ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, g_SwapChainResizeWidth, g_SwapChainResizeHeight, g_MinImageCount);
        });

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        io.DisplaySize = {1920, 1080};

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(window, true);
    }

    ~ImGuiInterface(){
        if(hasVulkanInited)
            ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    static void errorCallback(VkResult err){

        if(err != VK_SUCCESS)
            std::cerr << "ImGui Vulkan Error: " << err << std::endl << std::endl;
    
        return;
    }

    void render(VulkanCommandBuffer& commandBuffer){
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void initVulkan(std::shared_ptr<VulkanRenderPass> renderPass){
        
        ImGui_ImplVulkan_InitInfo init_info = getImguiInitInfo(renderPass);
        ImGui_ImplVulkan_Init(&init_info);
        hasVulkanInited = true;

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

private:

    ImGui_ImplVulkan_InitInfo getImguiInitInfo(std::shared_ptr<VulkanRenderPass> renderPass){

        ImGui_ImplVulkan_InitInfo init_info = {};
        //init_info.ApiVersion = VK_API_VERSION_1_3;
        init_info.Instance = *instance;
        init_info.PhysicalDevice = *physicalDevice;
        init_info.Device = *device;
        init_info.QueueFamily = physicalDevice->findQueueFamilies(*physicalDevice).graphicsFamily.value();
        init_info.Queue = device->getGraphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = *descriptorPool;
        init_info.RenderPass = *renderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = device->getSwapChain()->getMinImageCount();
        init_info.ImageCount = device->getSwapChain()->getImageCount();
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = errorCallback;

        return init_info;
    }

};


}
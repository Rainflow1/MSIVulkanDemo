#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanRenderPassI.h"
#include "vulkanMemory.h"

#include <iostream>
#include <vector>
#include <array>
#include <cstdint> 
#include <limits> 
#include <algorithm> 

namespace MSIVulkanDemo{



class VulkanFramebuffer{
private:
    std::shared_ptr<VulkanRenderPassI> renderPass;
    std::vector<std::shared_ptr<VulkanImageView>> imageViews;

    VkFramebuffer framebuffer = nullptr;
    uint32_t width, height; 

public:
    VulkanFramebuffer(std::shared_ptr<VulkanRenderPassI> renderPass, std::vector<std::shared_ptr<VulkanImageView>> imageViews): renderPass(renderPass), imageViews(imageViews){

        std::vector<VkImageView> attachments;

        for(auto view : imageViews){
            attachments.push_back(*view);
        }

        width = renderPass->getSwapChain()->getSwapChainExtent().width;
        height = renderPass->getSwapChain()->getSwapChainExtent().height;
    
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *renderPass;
        framebufferInfo.attachmentCount = attachments.size();
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = width;
        framebufferInfo.height = height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(*renderPass->getDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    ~VulkanFramebuffer(){
        if(framebuffer){
            vkDestroyFramebuffer(*renderPass->getDevice(), framebuffer, nullptr);
        }
    }

    std::pair<uint32_t, uint32_t> getResolution(){
        return {width, height};
    }

    void beginRenderPass(VkCommandBuffer& commandBuffer){
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = *renderPass;
        renderPassInfo.framebuffer = framebuffer;

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = renderPass->getSwapChain()->getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void recreateFramebuffer(std::vector<std::shared_ptr<VulkanImageView>> newImageViews){

        imageViews = newImageViews;

        if(framebuffer){
            vkDestroyFramebuffer(*renderPass->getDevice(), framebuffer, nullptr);
        }

        std::vector<VkImageView> attachments;

        for(auto view : imageViews){
            attachments.push_back(*view);
        }
    
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = renderPass->getSwapChain()->getSwapChainExtent().width;
        framebufferInfo.height = renderPass->getSwapChain()->getSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(*renderPass->getDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer!");
        }

    }

private:

};



}
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 

namespace MSIVulkanDemo{

class VulkanFramebuffer;

class VulkanImageView{
private:
    VulkanSwapChainI* swapChain;
    VkImage image;

    VkImageView imageView = nullptr;
    std::vector<std::weak_ptr<VulkanFramebuffer>> framebuffers;

public:
    VulkanImageView(): swapChain(nullptr), image(nullptr){

    }

    VulkanImageView(VulkanSwapChainI& swapChain, VkImage swapChainImage): swapChain(&swapChain), image(swapChainImage){
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImage;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapChain.getImageFormat();

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(*swapChain.getDevice(), &createInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }

    ~VulkanImageView(){
        for(auto framebuffer : framebuffers){
            framebuffer.reset();
        }

        if(imageView){
            vkDestroyImageView(*swapChain->getDevice(), imageView, nullptr);
        }
    }

    std::shared_ptr<VulkanFramebuffer> createFramebuffer(std::shared_ptr<VulkanSwapChainI> swapChain, VkRenderPass renderPass){
        std::shared_ptr<VulkanFramebuffer> fb = std::make_shared<VulkanFramebuffer>(swapChain, imageView, renderPass);
        for(auto framebuffer : framebuffers){
            if(framebuffer.expired()){
                framebuffer = fb;
                return fb;
            }
        }
        framebuffers.push_back(fb);
        return fb;
    }

};

class VulkanFramebuffer{
private:
    std::shared_ptr<VulkanSwapChainI> swapChain;
    VkImageView imageView;
    VkRenderPass renderPass;

    VkFramebuffer framebuffer = nullptr;    

public:
    VulkanFramebuffer(std::shared_ptr<VulkanSwapChainI> swapChain, VkImageView& imageView, VkRenderPass& renderPass): swapChain(swapChain), imageView(imageView), renderPass(renderPass){

        VkImageView attachments[] = {
            imageView
        };
    
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = swapChain->getSwapChainExtent().width;
        framebufferInfo.height = swapChain->getSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(*swapChain->getDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

    ~VulkanFramebuffer(){
        if(framebuffer){
            vkDestroyFramebuffer(*swapChain->getDevice(), framebuffer, nullptr);
        }
    }

    void beginRenderPass(VkCommandBuffer& commandBuffer){
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffer;

        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain->getSwapChainExtent();

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

private:

};

}
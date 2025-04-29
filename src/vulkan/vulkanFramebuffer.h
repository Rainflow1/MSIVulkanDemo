#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "vulkanShader.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 

namespace MSIVulkanDemo{


class VulkanGraphicsPipeline;


class VulkanRenderPass : public VulkanComponent<VulkanRenderPass>{
private:
    std::shared_ptr<VulkanSwapChainI> swapChain;

    VkRenderPass renderPass = nullptr;
    
    std::weak_ptr<VulkanGraphicsPipeline> graphicsPipeline; // TODO vector

public:

    VulkanRenderPass(std::shared_ptr<VulkanSwapChainI> swapChain): swapChain(swapChain){

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChain->getImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        //TODO other attachments (input, resolve, depth, stencil, preserve)

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(*swapChain->getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }

    }

    ~VulkanRenderPass(){
        if(renderPass){
            vkDestroyRenderPass(*swapChain->getDevice(), renderPass, nullptr);
        }
    }

    operator VkRenderPass() const{
        return renderPass;
    }

    std::shared_ptr<VulkanGraphicsPipeline> createGraphicsPipeline(std::vector<std::shared_ptr<VulkanShader>> shaders){
        if(graphicsPipeline.expired()){
            std::shared_ptr<VulkanGraphicsPipeline> gp = std::make_shared<VulkanGraphicsPipeline>(shared_from_this(), shaders);
            graphicsPipeline = gp;
            return gp;
        }else{
            return graphicsPipeline.lock();
        }
    }

    std::shared_ptr<VulkanDeviceI> getDevice(){
        return swapChain->getDevice();
    }

    std::shared_ptr<VulkanSwapChainI> getSwapChain(){
        return swapChain;
    }

};


class VulkanFramebuffer{
private:
    std::shared_ptr<VulkanRenderPass> renderPass;
    VkImageView imageView;
    

    VkFramebuffer framebuffer = nullptr;
    uint32_t width, height; 

public:
    VulkanFramebuffer(std::shared_ptr<VulkanRenderPass> renderPass, VkImageView& imageView): renderPass(renderPass), imageView(imageView){

        VkImageView attachments[] = {
            imageView
        };

        width = renderPass->getSwapChain()->getSwapChainExtent().width;
        height = renderPass->getSwapChain()->getSwapChainExtent().height;
    
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
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

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void recreateFramebuffer(VkImageView& newImageView){

        imageView = newImageView;

        if(framebuffer){
            vkDestroyFramebuffer(*renderPass->getDevice(), framebuffer, nullptr);
        }

        VkImageView attachments[] = {
            imageView
        };
    
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = *renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = renderPass->getSwapChain()->getSwapChainExtent().width;
        framebufferInfo.height = renderPass->getSwapChain()->getSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(*renderPass->getDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer!");
        }

    }

private:

};

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

    std::shared_ptr<VulkanFramebuffer> createFramebuffer(std::shared_ptr<VulkanRenderPass> renderPass){
        std::shared_ptr<VulkanFramebuffer> fb = std::make_shared<VulkanFramebuffer>(renderPass, imageView);
        for(auto framebuffer : framebuffers){
            if(framebuffer.expired()){
                framebuffer = fb;
                return fb;
            }
        }
        framebuffers.push_back(fb);
        return fb;
    }

    void recreateFramebuffer(VulkanImageView& oldImageView){
        for(auto fb : oldImageView.framebuffers){
            if(!fb.expired()){
                auto fbLock = fb.lock();
                fbLock->recreateFramebuffer(imageView);
                framebuffers.push_back(fbLock);
            }
        }
    }
};

}
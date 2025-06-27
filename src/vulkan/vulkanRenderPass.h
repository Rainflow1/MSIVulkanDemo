#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanRenderPassI.h"
#include "interface/vulkanSwapChainI.h"
#include "vulkanShader.h"
#include "vulkanCommandBuffer.h"
#include "vulkanSync.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 
#include <functional>
#include <deque>

namespace MSIVulkanDemo{


class VulkanGraphicsPipeline;
class VulkanFramebuffer;

class VulkanRenderPass: public VulkanComponent<VulkanRenderPass>, public VulkanRenderPassI{
private:
    std::shared_ptr<VulkanSwapChainI> swapChain;

    VkRenderPass renderPass = nullptr;
    
    std::vector<std::weak_ptr<VulkanGraphicsPipeline>> graphicsPipelines;
    std::vector<std::shared_ptr<VulkanFramebuffer>> framebuffers;

protected:
    
    std::map<std::string, std::shared_ptr<VulkanImageView>> imageViews;

    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> attachmentRefs;

    VkSubpassDependency dependency{};


public:

    VulkanRenderPass(std::shared_ptr<VulkanSwapChainI> swapChain): swapChain(swapChain){

        addAttachment(
            swapChain->getImageFormat(), 
            VK_SAMPLE_COUNT_1_BIT, 
            VK_ATTACHMENT_LOAD_OP_CLEAR, 
            VK_ATTACHMENT_STORE_OP_STORE, 
            VK_ATTACHMENT_LOAD_OP_DONT_CARE, 
            VK_ATTACHMENT_STORE_OP_DONT_CARE, 
            VK_IMAGE_LAYOUT_UNDEFINED, 
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );
        
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    }

    ~VulkanRenderPass(){
        if(renderPass){
            vkDestroyRenderPass(*swapChain->getDevice(), renderPass, nullptr);
        }
    }

    void createRenderPass(){

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
        subpass.colorAttachmentCount = 1; // TODO allow for more

        for(auto& ref : attachmentRefs){
            if(ref.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
                subpass.pDepthStencilAttachment = &ref;
            }
            if(ref.layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL){
                subpass.pColorAttachments = &ref;
            }
        }

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (VkResult errCode = vkCreateRenderPass(*swapChain->getDevice(), &renderPassInfo, nullptr, &renderPass); errCode != VK_SUCCESS) {
            throw std::runtime_error(std::format("failed to create render pass: {}", static_cast<int>(errCode)));
        }

        for(auto image : swapChain->getSwapChainImageViews()){
            std::vector<std::shared_ptr<VulkanImageView>> views;
            views.push_back(image);

            std::transform(imageViews.begin(), imageViews.end(), std::back_inserter(views), [](auto &kv){ return kv.second;});

            //views.insert(views.end(), imageViews.begin(), imageViews.end());

            framebuffers.push_back(std::shared_ptr<VulkanFramebuffer>(new VulkanFramebuffer(shared_from_this(), views)));
        }
    }

    operator VkRenderPass() const{
        return renderPass;
    }

    std::shared_ptr<VulkanGraphicsPipeline> createGraphicsPipeline(std::vector<std::shared_ptr<VulkanShader>> shaders){
        std::shared_ptr<VulkanGraphicsPipeline> gp = std::make_shared<VulkanGraphicsPipeline>(shared_from_this(), shaders);
        for(auto graphicsPipeline : graphicsPipelines){
            if(graphicsPipeline.expired()){
                graphicsPipeline = gp;
                return gp;
            }
        }
        graphicsPipelines.push_back(gp);
        return gp;
    }

    std::shared_ptr<VulkanDeviceI> getDevice(){
        return swapChain->getDevice();
    }

    std::shared_ptr<VulkanSwapChainI> getSwapChain(){
        return swapChain;
    }

    void addAttachment(VkFormat format, VkSampleCountFlagBits sampleCount, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkAttachmentLoadOp stencilLoadOp, VkAttachmentStoreOp stencilStoreOp, VkImageLayout initialLayout, VkImageLayout finalLayout, VkImageLayout layout){

        VkAttachmentDescription attachment{};
        VkAttachmentReference attachmentRef{};

        attachment.format = format;
        attachment.samples = sampleCount;
        attachment.loadOp = loadOp;
        attachment.storeOp = storeOp;
        attachment.stencilLoadOp = stencilLoadOp;
        attachment.stencilStoreOp = stencilStoreOp;
        attachment.initialLayout = initialLayout;
        attachment.finalLayout = finalLayout;

        attachmentRef.attachment = attachments.size();
        attachmentRef.layout = layout;

        attachments.push_back(attachment);
        attachmentRefs.push_back(attachmentRef);
    }

    void addImageView(std::string name, std::shared_ptr<VulkanImageView> imageView){
        imageViews.insert({name, imageView});
    }

    void addDependencyMask(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags srcAccessMask, VkPipelineStageFlags dstStageMask, VkPipelineStageFlags dstAccessMask){
        dependency.srcStageMask |= srcStageMask;
        dependency.srcAccessMask |= srcAccessMask;
        dependency.dstStageMask |= dstStageMask;
        dependency.dstAccessMask |= dstAccessMask;
    }

    std::shared_ptr<VulkanFramebuffer> getFramebuffer(uint32_t imageId){
        return framebuffers[imageId];
    }

    void recreateFramebuffers(VulkanSwapChainI& swapChain){

        for(auto [name, view] : imageViews){
            view->resize(std::pair<uint32_t, uint32_t>(swapChain.getSwapChainExtent().width, swapChain.getSwapChainExtent().height));
        }

        framebuffers.clear();
        for(auto image : swapChain.getSwapChainImageViews()){
            std::vector<std::shared_ptr<VulkanImageView>> views;
            views.push_back(image);

            std::transform(imageViews.begin(), imageViews.end(), std::back_inserter(views), [](auto &kv){ return kv.second;});

            //views.insert(views.end(), imageViews.begin(), imageViews.end());
            framebuffers.push_back(std::shared_ptr<VulkanFramebuffer>(new VulkanFramebuffer(shared_from_this(), views)));
        }
    }

};



}
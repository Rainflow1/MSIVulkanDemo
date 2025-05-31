#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanDeviceI.h"
#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanRenderPassI.h"
#include "vulkanShader.h"
#include "vulkanVertexData.h"
#include "vulkanUniform.h"

#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

namespace MSIVulkanDemo{


class VulkanGraphicsPipeline : public VulkanComponent<VulkanGraphicsPipeline>{
private:
    std::shared_ptr<VulkanRenderPassI> renderPass;
    //std::vector<std::shared_ptr<VulkanUniformLayout>> uniformLayouts;

    VkPipeline graphicsPipeline = nullptr;
    VkPipelineLayout pipelineLayout = nullptr;

    std::unique_ptr<VulkanUniformData> vertexUniforms;
    std::unique_ptr<VulkanUniformData> fragmentUniforms; 

public:
    VulkanGraphicsPipeline(std::shared_ptr<VulkanRenderPassI> renderPass, std::vector<std::shared_ptr<VulkanShader>> shaders): renderPass(renderPass){

        std::shared_ptr<VulkanShader> vertShader;
        std::shared_ptr<VulkanShader> fragShader;

        for(auto shader : shaders){
            if(shader->getType() == Vertex){
                vertShader = shader;
            }
            if(shader->getType() == Fragment){
                fragShader = shader;
            }
        }

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = *vertShader;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = *fragShader;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        VertexInputStateInfo vertexInputInfo = VertexInputStateInfo(vertShader->getVertexData());
        pipelineInfo.pVertexInputState = &vertexInputInfo.vertexInputInfo;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = getInputAssemblyInfo();
        pipelineInfo.pInputAssemblyState = &inputAssembly;

        ViewportStateInfo viewportState = ViewportStateInfo(*renderPass->getSwapChain());
        pipelineInfo.pViewportState = &viewportState.viewportState;

        VkPipelineRasterizationStateCreateInfo rasterizer = getRasterizerInfo();
        pipelineInfo.pRasterizationState = &rasterizer;

        VkPipelineMultisampleStateCreateInfo multisampling = getMultisamplingInfo();
        pipelineInfo.pMultisampleState = &multisampling;

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo = getDepthAndStencilInfo();
        pipelineInfo.pDepthStencilState = &depthStencilInfo;

        ColorBlendStateInfo colorBlending = ColorBlendStateInfo();
        pipelineInfo.pColorBlendState = &colorBlending.colorBlending;

        DynamicStateInfo dynamicState = DynamicStateInfo();
        pipelineInfo.pDynamicState = &dynamicState.dynamicState;

        pipelineInfo.renderPass = *renderPass;
        pipelineInfo.subpass = 0;


        vertexUniforms.reset(vertShader->getUniformData());
        fragmentUniforms.reset(fragShader->getUniformData());
        std::vector<std::shared_ptr<VulkanUniformLayout>> uniformLayouts = {(*vertexUniforms + *fragmentUniforms).getUniformLayout(renderPass->getDevice())};

        PipelineLayout pipeline = PipelineLayout(*renderPass->getSwapChain(), uniformLayouts);
        pipelineLayout = pipeline.pipelineLayout;
        pipelineInfo.layout = pipelineLayout;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(*renderPass->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

    ~VulkanGraphicsPipeline(){
        if(graphicsPipeline){
            vkDestroyPipeline(*renderPass->getDevice(), graphicsPipeline, nullptr);
        }

        if(pipelineLayout){
            vkDestroyPipelineLayout(*renderPass->getDevice(), pipelineLayout, nullptr);
        }
    }

    operator VkPipelineLayout() const{
        return pipelineLayout;
    }

    operator VkPipeline() const{
        return graphicsPipeline;
    }

    VulkanUniformData getUniformData(){
        return *vertexUniforms + *fragmentUniforms;
    }

    VulkanSwapChainI& getSwapChain(){
        return *renderPass->getSwapChain();
    }

    struct ViewportStateInfo{
        VkViewport viewport{};
        VkRect2D scissor{};
        VkPipelineViewportStateCreateInfo viewportState{};

        ViewportStateInfo(VulkanSwapChainI& swapChain){
            VkExtent2D swapChainExtent = swapChain.getSwapChainExtent();

            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChainExtent.width);
            viewport.height = static_cast<float>(swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            scissor.offset = {0, 0};
            scissor.extent = swapChainExtent;

            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;
        }
    };

private:

    struct DynamicStateInfo{
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};

        DynamicStateInfo(){
            
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

        }
    };

    struct VertexInputStateInfo{
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        VkVertexInputBindingDescription bindingDescription;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

        VertexInputStateInfo(const VulkanVertexData& vertices){
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            bindingDescription = vertices.getBindingDescription();
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertices.getAttributeDescriptions().size());
            attributeDescriptions = vertices.getAttributeDescriptions();
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        }
    };

    VkPipelineInputAssemblyStateCreateInfo getInputAssemblyInfo(){

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        return inputAssembly;
    }

    VkPipelineRasterizationStateCreateInfo getRasterizerInfo(){

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE; // TODO usefull for shadow maps
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        return rasterizer;
    }

    VkPipelineMultisampleStateCreateInfo getMultisamplingInfo(){

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        return multisampling;
    }

    VkPipelineDepthStencilStateCreateInfo getDepthAndStencilInfo(){
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        return depthStencil;
    }

    struct ColorBlendStateInfo{
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo colorBlending{};

        ColorBlendStateInfo(){
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE; // TODO alpha blending turned off for now
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
        }
    };

    struct PipelineLayout{
        
        VkDescriptorSetLayout layout;
        VkPipelineLayout pipelineLayout = nullptr;

        PipelineLayout(VulkanSwapChainI& swapChain, std::vector<std::shared_ptr<VulkanUniformLayout>> uniformLayouts){
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(uniformLayouts.size());
            
            std::vector<VkDescriptorSetLayout> layouts;
            for(auto uniformLayout : uniformLayouts){
                layouts.push_back(*uniformLayout);
            }

            pipelineLayoutInfo.pSetLayouts = layouts.data();
            
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            if (vkCreatePipelineLayout(*swapChain.getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        }
    };

};



}
#pragma once

#include "vulkanDeviceI.h"

namespace MSIVulkanDemo{

class VulkanGraphicsPipeline;
class VulkanSwapChainI;
class VulkanShader;

class VulkanRenderPassI{
public:
    virtual ~VulkanRenderPassI(){};
    virtual operator VkRenderPass() const = 0;
    virtual std::shared_ptr<VulkanGraphicsPipeline> createGraphicsPipeline(std::vector<std::shared_ptr<VulkanShader>>) = 0;
    virtual std::shared_ptr<VulkanDeviceI> getDevice() = 0;
    virtual std::shared_ptr<VulkanSwapChainI> getSwapChain() = 0;
    virtual void recreateFramebuffers(VulkanSwapChainI&) = 0;
};

}
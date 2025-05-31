#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vulkanBufferI.h"

namespace MSIVulkanDemo{

class VulkanCommandBufferI{
public:
    virtual ~VulkanCommandBufferI(){};
    virtual VulkanCommandBufferI& begin(VkCommandBufferUsageFlags) = 0;
    virtual VulkanCommandBufferI& copyBuffer(VulkanBufferI& src, VulkanBufferI& dst, VkDeviceSize size) = 0;
    virtual VulkanCommandBufferI& copyBufferToImage(VulkanBufferI& src, VulkanImageI& dst, VkBufferImageCopy& region) = 0;
    virtual VulkanCommandBuffer& setBarrier(VkImageMemoryBarrier& barrier, VkPipelineStageFlags srcStage = 0, VkPipelineStageFlags dstStage = 0) = 0;
    virtual VulkanCommandBufferI& end() = 0;
    virtual VulkanCommandBufferI& submit() = 0;
    virtual operator VkCommandBuffer() const = 0;
};

};
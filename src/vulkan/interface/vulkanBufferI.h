#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBufferI;
class VulkanGraphicsPipeline;

class VulkanBufferI{
public:
    virtual ~VulkanBufferI(){};
    virtual operator VkBuffer() const = 0;
    virtual void bind(VulkanCommandBufferI&) const = 0;
};

};
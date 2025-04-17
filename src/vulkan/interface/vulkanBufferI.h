#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBuffer;
class VulkanGraphicsPipeline;

class VulkanBufferI{
public:
    virtual ~VulkanBufferI(){};
    virtual operator VkBuffer() const = 0;
    virtual void bind(VulkanCommandBuffer&, std::shared_ptr<VulkanGraphicsPipeline>) const = 0;
};

};
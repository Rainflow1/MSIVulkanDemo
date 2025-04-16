#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBuffer;

class VulkanBufferI{
public:
    virtual ~VulkanBufferI(){};
    virtual operator VkBuffer() const = 0;
    virtual void bind(VulkanCommandBuffer&) const = 0;
};

};
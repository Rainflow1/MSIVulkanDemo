#pragma once

namespace MSIVulkanDemo{

class VulkanBufferI{
public:
    virtual ~VulkanBufferI(){};
    virtual operator VkBuffer() const = 0;
};

};
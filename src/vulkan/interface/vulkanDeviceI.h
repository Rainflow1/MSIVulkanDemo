#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBuffer;

class VulkanDeviceI{
public:
    virtual ~VulkanDeviceI(){};
    virtual VulkanPhysicalDevice& getPhysicalDevice() = 0;
    virtual operator VkDevice() const = 0;
    virtual VkDevice getDevice() = 0;
    virtual VkQueue getGraphicsQueue() = 0;
    virtual VkQueue getPresentQueue() = 0;
    virtual std::shared_ptr<VulkanCommandBuffer> createCommandBuffer() = 0;
};

};
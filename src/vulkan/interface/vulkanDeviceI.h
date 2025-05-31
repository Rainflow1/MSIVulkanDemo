#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBuffer;
class VulkanMemoryManager;
class VulkanDescriptorPool;
class VulkanSemaphore;
class VulkanFence;
class VulkanTextureSampler;

class VulkanDeviceI{
public:
    virtual ~VulkanDeviceI(){};
    virtual VulkanPhysicalDevice& getPhysicalDevice() = 0;
    virtual operator VkDevice() const = 0;
    virtual VkDevice getDevice() = 0;
    virtual VkQueue getGraphicsQueue() = 0;
    virtual VkQueue getPresentQueue() = 0;
    virtual std::shared_ptr<VulkanCommandBuffer> createCommandBuffer() = 0;
    virtual std::shared_ptr<VulkanMemoryManager> createMemoryManager() = 0;
    virtual std::shared_ptr<VulkanMemoryManager> getMemoryManager() = 0;
    virtual std::shared_ptr<VulkanSemaphore> createSemaphore() = 0;
    virtual std::shared_ptr<VulkanFence> createFence(bool = false) = 0;
    virtual std::shared_ptr<VulkanTextureSampler> createTextureSampler() = 0;
    virtual std::shared_ptr<VulkanDescriptorPool> createDescriptorPool(std::vector<std::pair<VkDescriptorType, uint32_t>> = {}) = 0;
};

};
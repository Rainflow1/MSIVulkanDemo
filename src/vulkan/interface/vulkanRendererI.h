#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBuffer;

class VulkanRendererI{
public:
    virtual ~VulkanRendererI(){};
    virtual void render(VulkanCommandBuffer&) = 0;
};

};
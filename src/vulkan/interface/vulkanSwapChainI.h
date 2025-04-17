#pragma once

#include "vulkanDeviceI.h"

namespace MSIVulkanDemo{

class VulkanSwapChainI{
public:
    virtual ~VulkanSwapChainI(){};
    virtual VkExtent2D getSwapChainExtent() = 0;
    virtual VkFormat getImageFormat() = 0;
    virtual std::shared_ptr<VulkanDeviceI> getDevice() = 0;
    virtual std::vector<VkImage>& getSwapChainImages() = 0;
};

}
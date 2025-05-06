#pragma once

#include "vulkanDeviceI.h"

#include <functional>

namespace MSIVulkanDemo{

class VulkanImageView;
class VulkanSemaphore;

class VulkanSwapChainI{
public:
    virtual ~VulkanSwapChainI(){};
    virtual VkExtent2D getSwapChainExtent() = 0;
    virtual VkFormat getImageFormat() = 0;
    virtual std::shared_ptr<VulkanDeviceI> getDevice() = 0;
    virtual std::vector<VkImage>& getSwapChainImages() = 0;
    virtual std::vector<std::shared_ptr<VulkanImageView>>& getSwapChainImageViews() = 0;
    virtual uint32_t getNextImage(VulkanSemaphore&) = 0;
    virtual void presentImage(VulkanSemaphore&, uint32_t) = 0;
    virtual void addSwapChainRecreateCallback(std::function<void(VulkanSwapChainI&)> cb) = 0;
    virtual uint32_t getMinImageCount() = 0;
};

}
#pragma once

namespace MSIVulkanDemo{



class VulkanImageI{
public:
    virtual ~VulkanImageI(){};
    virtual operator VkImage() const = 0;
};

};
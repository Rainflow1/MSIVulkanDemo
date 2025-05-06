#pragma once

namespace MSIVulkanDemo{

class VulkanDescriptorSet;
class VulkanGraphicsPipeline;

class VulkanDescriptorSetOwner{
public:
    virtual void setDescriptorSet(std::vector<std::shared_ptr<VulkanDescriptorSet>>) = 0;
    virtual std::shared_ptr<VulkanGraphicsPipeline> getGraphicsPipeline() = 0;
    virtual std::vector<std::shared_ptr<VulkanDescriptorSet>> getDescriptorSet() = 0;
};

};
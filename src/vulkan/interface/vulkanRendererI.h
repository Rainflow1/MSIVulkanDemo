#pragma once

namespace MSIVulkanDemo{

class VulkanCommandBuffer;
class VulkanRenderGraph;

class VulkanRendererI{
public:
    virtual ~VulkanRendererI(){};
    virtual void render(VulkanCommandBuffer&) = 0;
};

class VulkanRenderGraphBuilderI{
public:
    virtual ~VulkanRenderGraphBuilderI(){};
    virtual void buildRenderGraph(std::shared_ptr<VulkanRenderGraph>) = 0;
};

};
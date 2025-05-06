#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "interface/vulkanSwapChainI.h"
#include "interface/vulkanDescriptorSetOwner.h"
#include "vulkanSync.h"
#include "vulkanRenderPass.h"
#include "vulkanMemory.h"

#include <iostream>
#include <vector>
#include <cstdint> 
#include <limits> 
#include <algorithm> 
#include <functional>
#include <deque>

namespace MSIVulkanDemo{



class VulkanRenderGraph : public VulkanComponent<VulkanRenderGraph>{

    class Dependency;

    class RenderGraphNode: public VulkanRenderPass{
    private:
        std::weak_ptr<VulkanRenderGraph> renderGraph;
        std::shared_ptr<VulkanSwapChainI> swapChain;
        std::string name;

        std::vector<std::shared_ptr<Dependency>> dependencies;

        bool isBaked = false;

        std::shared_ptr<std::function<void(VulkanCommandBuffer&)>> renderFunction;

        bool isOutputTarget = false;

        std::string inputName;
        bool hasInputTarget = false;
        std::weak_ptr<RenderGraphNode> inputNode;

    public:
        RenderGraphNode(std::shared_ptr<VulkanRenderGraph> renderGraph, std::shared_ptr<VulkanSwapChainI> swapChain, std::string name): VulkanRenderPass(swapChain), renderGraph(renderGraph), swapChain(swapChain), name(name){

            swapChain->addSwapChainRecreateCallback([&](VulkanSwapChainI& swapChain){
                this->recreateFramebuffers(swapChain);
            });

        }
        ~RenderGraphNode(){}

        std::function<void(VulkanCommandBuffer&)>& getRenderFunction(){
            return *renderFunction;
        }

        void setRenderFunction(std::function<void(VulkanCommandBuffer&)>& fun){
            renderFunction = std::shared_ptr<std::function<void(VulkanCommandBuffer&)>>(new std::function<void(VulkanCommandBuffer&)>(fun));
        }

        void bake(VulkanSwapChainI& swapChain){
            
            if(hasInputTarget){
                inputNode = renderGraph.lock()->getNode(inputName);
            }

            // TODO validate ? 
            createRenderPass();
            isBaked = true;
        }

        void addDependency(std::shared_ptr<Dependency> dependency){
            dependencies.push_back(dependency);
        }

        void markOutput(){
            isOutputTarget = true;
            attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // FIXME
        }

        bool isMarkedAsOutput(){
            return isOutputTarget;
        }

        void setInput(std::string nodeName){
            inputName = nodeName;
            hasInputTarget = true;
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  // FIXME
        }

        std::shared_ptr<RenderGraphNode> getInputNode(){
            if(inputNode.expired()){
                return nullptr;
            }
            return inputNode.lock();
        }

    };


    enum DependencyType{ // TODO probably unnecessary
        None,
        RenderFunction
    };


    class Dependency{
        DependencyType type = None;

    public:
        Dependency(DependencyType type): type(type){}
        virtual ~Dependency(){};

        DependencyType getType(){
            return type;
        }

        virtual void apply(RenderGraphNode&) = 0;
        virtual Dependency* clone() const = 0;
    };


private:
    std::shared_ptr<VulkanSwapChainI> swapChain;

    std::map<std::string, std::shared_ptr<RenderGraphNode>> nodes;
    std::deque<std::shared_ptr<RenderGraphNode>> nodesQueue;

    bool isBaked = false;

    std::vector<std::shared_ptr<VulkanCommandBuffer>> commandBuffers;
    std::vector<std::shared_ptr<VulkanSemaphore>> imageAvailableSemaphores;
    std::vector<std::shared_ptr<VulkanSemaphore>> renderFinishedSemaphores;
    std::vector<std::shared_ptr<VulkanFence>> inFlightFences;

public:
    VulkanRenderGraph(std::shared_ptr<VulkanSwapChainI> swapChain): swapChain(swapChain){
        // TODO max frames in flight depended
        commandBuffers = {swapChain->getDevice()->createCommandBuffer(), swapChain->getDevice()->createCommandBuffer()};
        imageAvailableSemaphores = {swapChain->getDevice()->createSemaphore(), swapChain->getDevice()->createSemaphore()};
        renderFinishedSemaphores = {swapChain->getDevice()->createSemaphore(), swapChain->getDevice()->createSemaphore()};
        inFlightFences = {swapChain->getDevice()->createFence(true), swapChain->getDevice()->createFence(true)};
    }

    ~VulkanRenderGraph(){}

    template<class... D>
    typename std::enable_if<(std::is_base_of<Dependency, D>::value && ...)>::type
    addRenderPass(std::string name, D&... args){
        std::array<std::shared_ptr<Dependency>, sizeof...(D)> dependencies = {{std::move(std::shared_ptr<Dependency>(args.clone())) ...}};

        std::shared_ptr<RenderGraphNode> node = std::make_shared<RenderGraphNode>(shared_from_this(), swapChain, name);

        for(std::shared_ptr<Dependency>& dependency : dependencies){
            node->addDependency(dependency);
            dependency->apply(*node);
        }

        nodes.insert({name, node});
    }

    void bake(){
        validate();

        std::shared_ptr<RenderGraphNode> outputNode;

        for(auto [name, node] : nodes){
            node->bake(*swapChain);
            if(node->isMarkedAsOutput()){
                if(outputNode){
                    throw std::runtime_error("RenderGraph cant have mora than one output node");
                }
                outputNode = node;
            }
        }

        if(!outputNode){
            throw std::runtime_error("RenderGraph needs a output node"); // TODO make renderGraph own exception class, and throw them from appriopriate classes
        }

        

        std::shared_ptr<RenderGraphNode> nextNode = outputNode;

        while(nextNode){
            nodesQueue.push_front(nextNode);
            nextNode = nextNode->getInputNode();
        }

        isBaked = true;
    }

    void validate(){
        // TODO validate
    }

    void render(uint64_t frameIndex){

        inFlightFences[frameIndex]->reset();

        commandBuffers[frameIndex]->reset();

        uint32_t imageId = swapChain->getNextImage(*imageAvailableSemaphores[frameIndex]);

        if(imageId == -1){
            return;
        }

        for(auto node : nodesQueue){
            // TODO synch
            //auto frameBuffer = swapChain->getFramebuffer(imageId, std::static_pointer_cast<VulkanRenderPass>(node));
            auto frameBuffer = node->getFramebuffer(imageId);
            commandBuffers[frameIndex]->beginRenderPass(frameBuffer);
            node->getRenderFunction()(*commandBuffers[frameIndex]);
            commandBuffers[frameIndex]->endRenderPass();

        }
        commandBuffers[frameIndex]->submit(*imageAvailableSemaphores[frameIndex], *renderFinishedSemaphores[frameIndex], *inFlightFences[frameIndex]);

        swapChain->presentImage(*renderFinishedSemaphores[frameIndex], imageId);
        inFlightFences[frameIndex]->waitFor();
    }
/*
    std::vector<std::shared_ptr<VulkanDescriptorSet>> registerDescriptorSet(VulkanUniformData& uniformData){
        std::vector<std::shared_ptr<VulkanDescriptorSet>> sets;
        for(auto buffer : commandBuffers){
            sets.push_back(buffer->createDescriptorSet(uniformData));
        }
        return sets;
    }
*/
    void registerDescriptorSet(VulkanDescriptorSetOwner* owner){
        std::vector<std::shared_ptr<VulkanDescriptorSet>> sets;
        if(owner->getDescriptorSet().size() > 0){
            return;
        }
        for(auto buffer : commandBuffers){
            sets.push_back(buffer->createDescriptorSet(owner->getGraphicsPipeline()->getUniformData()));
        }
        owner->setDescriptorSet(sets);
    }

    std::shared_ptr<VulkanRenderPass> getRenderPass(std::string name){
        if(!isBaked){
            throw std::runtime_error("Renderpass needs to be baked");
        }

        return std::static_pointer_cast<VulkanRenderPass>(nodes[name]);
    }

private:

    std::shared_ptr<RenderGraphNode> getNode(std::string name){
        return nodes[name];
    }

public:
    class DepthOnly : public Dependency{
    public:
        DepthOnly(): Dependency(None){}
        DepthOnly(const DepthOnly& other): Dependency(None){}
        ~DepthOnly(){}

        void apply(RenderGraphNode& node){}
        Dependency* clone() const{return new DepthOnly(*this);}
    };

    class AddInput : public Dependency{
    public:
        AddInput(std::string a, std::string b): Dependency(None){}
        AddInput(const AddInput& other): Dependency(None){}
        ~AddInput(){}

        void apply(RenderGraphNode& node){}
        Dependency* clone() const{return new AddInput(*this);}
    };

    class AddRenderFunction : public Dependency{
    private:
        std::function<void(VulkanCommandBuffer&)> fun;

    public:
        AddRenderFunction(std::function<void(VulkanCommandBuffer&)> fun): Dependency(RenderFunction), fun(std::move(fun)){}
        AddRenderFunction(const AddRenderFunction& other): Dependency(RenderFunction), fun(other.fun){}
        ~AddRenderFunction(){}

        void apply(RenderGraphNode& node){
            node.setRenderFunction(fun);
        }
        Dependency* clone() const{return new AddRenderFunction(*this);}
    };

    class SetRenderTargetInput : public Dependency{
    private:
        std::string nodeName;

    public:
        SetRenderTargetInput(std::string nodeName): Dependency(None), nodeName(nodeName){}
        SetRenderTargetInput(const SetRenderTargetInput& other): Dependency(RenderFunction), nodeName(other.nodeName){}
        ~SetRenderTargetInput(){}

        void apply(RenderGraphNode& node){
            node.setInput(nodeName);
        }
        Dependency* clone() const{return new SetRenderTargetInput(*this);}
    };

    class SetRenderTargetOutput : public Dependency{
    public:
        SetRenderTargetOutput(): Dependency(None){}
        ~SetRenderTargetOutput(){}

        void apply(RenderGraphNode& node){
            node.markOutput();
        }
        Dependency* clone() const{return new SetRenderTargetOutput(*this);}
    };

    class AddDepthBuffer : public Dependency{
    private:
        std::shared_ptr<VulkanImageView> depthView;

    public:
        AddDepthBuffer(): Dependency(None){}
        ~AddDepthBuffer(){}

        void apply(RenderGraphNode& node){
            
            VkFormat depthFormat = node.getSwapChain()->getDevice()->getPhysicalDevice().findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

            // TODO has stencil component https://vulkan-tutorial.com/Depth_buffering

            std::shared_ptr<VulkanImage> depthImage = node.getSwapChain()->getDevice()->getMemoryManager()->createImage<VulkanImage>(
                std::pair<uint32_t, uint32_t>({node.getSwapChain()->getSwapChainExtent().width, node.getSwapChain()->getSwapChainExtent().height}),
                depthFormat, 
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
            
            depthView = depthImage->createImageView(VK_IMAGE_ASPECT_DEPTH_BIT);

            node.addAttachment(depthFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            node.addDependencyMask(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

            node.addImageView(depthView);
        }
        Dependency* clone() const{return new AddDepthBuffer(*this);}
    };

};



}
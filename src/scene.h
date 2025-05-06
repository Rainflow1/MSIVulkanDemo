#pragma once

#include <entt/entity/registry.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "component.h"
#include "resourceManager.h"
#include "vulkan/vulkanCore.h"

#include <iostream>
#include <vector>
#include <type_traits>
#include <typeinfo>

namespace MSIVulkanDemo{


class GameObject{

private:
    std::shared_ptr<entt::registry> entityRegistry;
    const entt::entity entityID;

public:
    GameObject(std::shared_ptr<entt::registry> entityRegistry): entityRegistry(entityRegistry), entityID(entityRegistry->create()){
        
    }

    ~GameObject(){
        if(entityRegistry && entityRegistry->valid(entityID))
            entityRegistry->destroy(entityID);
    }

    GameObject(GameObject& other): entityRegistry(other.entityRegistry), entityID(other.entityID){
        
    }

    GameObject(GameObject&& other): entityRegistry(other.entityRegistry), entityID(other.entityID){
        other.entityRegistry = nullptr;
    }

    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value>::type
    addComponent(Args&... args){
        entityRegistry->emplace<T>(entityID, args...);
    }

    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value, T&>::type
    getComponent(){

        if(!entityRegistry->all_of<T>(entityID)){
            throw std::runtime_error(std::string("Entity does not have component: ") + typeid(T).name());
        }

        return entityRegistry->get<T>(entityID);
    }

    template<typename T, typename... Args>
    typename std::enable_if<std::is_base_of<Component, T>::value>::type
    removeComponent(){
        entityRegistry->remove<T>(entityID);
    }

};


class Scene : public VulkanRendererI, public VulkanRenderGraphBuilderI{

private:
    std::vector<GameObject> gameObjects;
    std::shared_ptr<entt::registry> entityRegistry;
    std::shared_ptr<ImGuiInterface> gui;

    std::shared_ptr<VulkanRenderGraph> renderGraph;
    std::shared_ptr<VulkanRenderPass> mainRenderpass;

    std::chrono::steady_clock::time_point previousTime = std::chrono::high_resolution_clock::now();

protected:
    ResourceManager resourceManager;

public:
    Scene(){
        entityRegistry = std::shared_ptr<entt::registry>(new entt::registry());
    }

    Scene(Scene& other) = delete;

    virtual ~Scene(){

        entityRegistry->clear();
    }

    virtual void setup() = 0;

    virtual void update() = 0;

    void buildRenderGraph(std::shared_ptr<VulkanRenderGraph> renderGraph){
        //renderGraph.addRenderPass("test");
        //renderGraph.addRenderPass("Shadow", 
        //    VulkanRenderGraph::DepthOnly()
        //);
        renderGraph->addRenderPass("Main", 
            //VulkanRenderGraph::AddInput("Shadow", "ShadowMap"), 
            VulkanRenderGraph::AddRenderFunction([&](VulkanCommandBuffer& commandBuffer){
                this->render(commandBuffer);
            }),
            VulkanRenderGraph::AddDepthBuffer()
        );

        if(this->gui){

            renderGraph->addRenderPass("UI", 
                VulkanRenderGraph::SetRenderTargetInput("Main"), 
                VulkanRenderGraph::SetRenderTargetOutput(),
                VulkanRenderGraph::AddRenderFunction([&](VulkanCommandBuffer& commandBuffer){
                    this->gui->render(commandBuffer);
                })
            );

        }
        
        renderGraph->bake(); // TODO consider who should bake the graph

        this->renderGraph = renderGraph;
        mainRenderpass = renderGraph->getRenderPass("Main");
        gui->initVulkan(renderGraph->getRenderPass("UI"));
    }

    void render(VulkanCommandBuffer& commandBuffer){

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float totalTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();
        static uint32_t frames = 0, totalFrames = 0;
        static float cumulativeTime = 0.0f;
        cumulativeTime += deltaTime;
        frames++;
        totalFrames++;
        
        if(cumulativeTime >= 1.0f){
            cumulativeTime = 0.0f;
            std::cout << frames << ", " << totalFrames/totalTime << ", " << deltaTime << std::endl;
            frames = 0;
        }
        

        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), commandBuffer.getWidth() / (float) commandBuffer.getHeight(), 0.1f, 10.0f);
        proj[1][1] *= -1;

        auto entityView = entityRegistry->view<RenderComponent, ModelComponent, MaterialComponent, TransformComponent>();

        for(auto entity : entityView){
            
            glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), entityView.get<TransformComponent>(entity).getPosition()), totalTime * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            commandBuffer
            .bind(entityView.get<MaterialComponent>(entity).getGraphicsPipeline())
            .bind(entityView.get<ModelComponent>(entity).getBuffers())
            .bind(entityView.get<MaterialComponent>(entity).getDescriptorSet())
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("model", model))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("view", view))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("proj", proj))
            .draw(entityView.get<ModelComponent>(entity).getCount());

        }

        previousTime = currentTime;
    }

    void loadScene(Vulkan& context){

        setup();

        
        resourceManager.addDependency<ShaderProgram>(mainRenderpass);
        resourceManager.addDependency<Mesh>(context.getMemoryManager());

        auto entityView = entityRegistry->view<MaterialComponent>();

        for(auto [entity, mat] : entityView.each()){
            mat.setDescriptorSet(renderGraph->registerDescriptorSet(mat.getGraphicsPipeline()->getUniformData())); // TODO make that indepented of scene/renderGraph load order
        }
    }

    void unloadScene(){
        return;
    }

    void loadGui(std::shared_ptr<ImGuiInterface> gui){
        this->gui = gui;
    }

protected:
    GameObject& spawnGameObject(){
        return gameObjects.emplace_back(entityRegistry); // TODO add names to gameobjects
    }



private:


};


class SimpleScene : public Scene{

public:
    SimpleScene() : Scene(){ // TODO no consturctor

    }

    ~SimpleScene(){

    }

    void setup(){
        
        GameObject& obj = spawnGameObject();
        obj.addComponent<MaterialComponent>(resourceManager.getResource<ShaderProgram>("./shaders/tak.glsl"));
        obj.addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/tak.cos"));
        obj.addComponent<TransformComponent>(glm::vec3(1.0f, 1.0f, 0.0f));
        obj.addComponent<RenderComponent>();

        GameObject& obj2 = spawnGameObject();
        obj2.addComponent<MaterialComponent>(resourceManager.getResource<ShaderProgram>("./shaders/tak.glsl"));
        obj2.addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/tak.cos"));
        obj2.addComponent<TransformComponent>(glm::vec3(1.0f, -1.0f, 0.0f));
        obj2.addComponent<RenderComponent>();
    }

    void update(){
        
    }

};

}
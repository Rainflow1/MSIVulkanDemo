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
    std::map<std::string, std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<entt::registry> entityRegistry;
    std::shared_ptr<ImGuiInterface> gui;

    std::shared_ptr<VulkanRenderGraph> renderGraph;
    std::shared_ptr<VulkanRenderPass> mainRenderpass;

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

    virtual void update(float deltaTime) = 0;

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

        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), commandBuffer.getWidth() / (float) commandBuffer.getHeight(), 0.1f, 10.0f);
        proj[1][1] *= -1;

        auto entityView = entityRegistry->view<RenderComponent, ModelComponent, MaterialComponent, TransformComponent>();

        for(auto entity : entityView){
            
            auto transform = entityView.get<TransformComponent>(entity);

            glm::mat4 model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), transform.getPosition()), transform.getRotationAngle(), transform.getRotationVec()), transform.getScale());

            renderGraph->registerDescriptorSet(&entityView.get<MaterialComponent>(entity));

            commandBuffer
            .bind(entityView.get<MaterialComponent>(entity).getGraphicsPipeline())
            .bind(entityView.get<ModelComponent>(entity).getBuffers())
            .bind(entityView.get<MaterialComponent>(entity).getDescriptorSet())
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("model", model))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("view", view))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("proj", proj))
            .draw(entityView.get<ModelComponent>(entity).getCount());

        }

    }

    void loadScene(Vulkan& context){
        resourceManager.addDependency<ShaderProgram>(mainRenderpass);
        resourceManager.addDependency<Mesh>(context.getMemoryManager());

        setup();
/*
        auto entityView = entityRegistry->view<MaterialComponent>();

        for(auto [entity, mat] : entityView.each()){
            mat.setDescriptorSet(renderGraph->registerDescriptorSet(mat.getGraphicsPipeline()->getUniformData())); // TODO make that indepented of scene/renderGraph load order
        }
*/
    }

    void unloadScene(){
        return;
    }

    void loadGui(std::shared_ptr<ImGuiInterface> gui){
        this->gui = gui;
    }

protected:
    GameObject* spawnGameObject(std::string objName){
        std::shared_ptr<GameObject> ptr = std::make_shared<GameObject>(entityRegistry);
        gameObjects.insert({objName, ptr});
        return &*ptr;
    }

    GameObject* getGameObject(std::string objName){     
        if(gameObjects.find(objName) == gameObjects.end()){
            return nullptr;
        }
        std::shared_ptr<GameObject> ptr = gameObjects[objName];
        return &*ptr;
    }

private:


};


class SimpleScene : public Scene{

public:

    void setup(){
        
        GameObject* obj = spawnGameObject("Object1");
        obj->addComponent<MaterialComponent>(resourceManager.getResource<ShaderProgram>("./shaders/tak.glsl"));
        obj->addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/tak.cos"));
        obj->addComponent<TransformComponent>(
            glm::vec3(-1.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(1.0f, 1.0f, 1.0f)
        );
        obj->addComponent<RenderComponent>();

    }

    void update(float deltaTime){
        
        static float totalTime = 0.0;
        totalTime += deltaTime;

        if(!getGameObject("Object2")){
            GameObject* obj2 = spawnGameObject("Object2");
            obj2->addComponent<MaterialComponent>(resourceManager.getResource<ShaderProgram>("./shaders/tak.glsl"));
            obj2->addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/tak.cos"));
            obj2->addComponent<TransformComponent>(
                glm::vec3(0.5f, 0.0f, 0.0f), 
                glm::vec3(0.0f, 0.0f, 0.0f), 
                glm::vec3(0.2f, 0.2f, 0.2f)
            );
            obj2->addComponent<RenderComponent>();
        }

        GameObject* obj = getGameObject("Object1");

        obj->getComponent<TransformComponent>().setRotation(glm::radians(45.0f) * totalTime, glm::vec3(0.0f, 0.0f, 1.0f));
        
    }

};

}
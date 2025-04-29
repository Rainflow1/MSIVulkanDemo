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


class Scene : public VulkanRendererI{

private:
    std::vector<GameObject> gameObjects;
    std::shared_ptr<entt::registry> entityRegistry;

protected:
    ResourceManager resourceManager;

public:
    Scene(){
        entityRegistry = std::shared_ptr<entt::registry>(new entt::registry());
    }

    virtual ~Scene(){

        entityRegistry->clear();
    }

    virtual void setup() = 0;

    virtual void update() = 0;

    void render(VulkanCommandBuffer& commandBuffer){

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        
        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), commandBuffer.getWidth() / (float) commandBuffer.getHeight(), 0.1f, 10.0f);
        proj[1][1] *= -1;

        auto entityView = entityRegistry->view<RenderComponent, ModelComponent, MaterialComponent, TransformComponent>();

        for(auto entity : entityView){
            
            glm::mat4 model = glm::rotate(glm::translate(glm::mat4(1.0f), entityView.get<TransformComponent>(entity).getPosition()), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

            commandBuffer
            .bind(entityView.get<MaterialComponent>(entity).getGraphicsPipeline())
            .bind(entityView.get<ModelComponent>(entity).getBuffers())
            .bind(entityView.get<MaterialComponent>(entity).getDescriptorSet())
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("model", model))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("view", view))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("proj", proj))
            .draw(entityView.get<ModelComponent>(entity).getCount());

        }

/*
        commandBuffer
        .bind(graphicsPipeline) // TODO foreach material component 
        .bind(*vertexBuffer) // TODO foreach model component
        .bind(*indexBuffer) // TODO foreach model component
        .bind(*uniformBuffers[frameIndex]) // TODO done during pipeline bind
        .uniform<glm::mat4>(model)
        .uniform<glm::mat4>(view) // TODO foreach transform component
        .uniform<glm::mat4>(proj)
        .draw(vertexBuffer->getVertexCount(), indexBuffer->getIndexCount()); // TODO foreach render component
*/
    }

    void loadScene(Vulkan& context){

        setup();

        std::shared_ptr<VulkanRenderPass> renderpass = context.getRenderPass();
        resourceManager.addDependency<ShaderProgram>(renderpass);
        resourceManager.addDependency<Mesh>(context.getMemoryManager());

        auto entityView = entityRegistry->view<MaterialComponent>();

        for(auto [entity, mat] : entityView.each()){
            mat.setDescriptorSet(context.registerDescriptorSet(mat.getGraphicsPipeline()->getUniformData()));
        }
    }

    void unloadScene(){
        return;
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
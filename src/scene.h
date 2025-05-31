#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "resourceManager.h"
#include "vulkan/vulkanCore.h"
#include "input.h"
#include "gameobject.h"

#include <iostream>
#include <vector>
#include <type_traits>
#include <typeinfo>

namespace MSIVulkanDemo{


class Scene : public VulkanRendererI, public VulkanRenderGraphBuilderI{

private:
    std::map<std::string, std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<entt::registry> entityRegistry;
    std::shared_ptr<ImGuiInterface> gui;

    std::shared_ptr<VulkanRenderGraph> renderGraph;
    std::shared_ptr<VulkanRenderPass> mainRenderpass;

    GameObject* mainCamera;

    std::shared_ptr<Texture> defaultTexture;

protected:
    ResourceManager resourceManager;

public:
    Scene(){
        entityRegistry = std::shared_ptr<entt::registry>(new entt::registry());
        mainCamera = spawnGameObject("MainCamera");
        mainCamera->addComponent<TransformComponent>(glm::vec3(-2.0f, 0.0f, 1.0f));
        mainCamera->addComponent<CameraComponent>();
    }

    Scene(Scene& other) = delete;

    virtual ~Scene(){

        entityRegistry->clear();
    }

    virtual void setup() = 0;

    void updateScene(float deltaTime, Input& input){

        auto& camera = mainCamera->getComponent<CameraComponent>();
        camera.provideInput(deltaTime, input);

        return this->update(deltaTime, input);
    }

    virtual void update(float deltaTime, Input& input) = 0;

    void buildRenderGraph(std::shared_ptr<VulkanRenderGraph> renderGraph){

        renderGraph->addRenderPass("Main", 
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
        
        renderGraph->bake();

        this->renderGraph = renderGraph;
        mainRenderpass = renderGraph->getRenderPass("Main");
        gui->initVulkan(renderGraph->getRenderPass("UI"));
    }

    void render(VulkanCommandBuffer& commandBuffer){

        auto& camera = mainCamera->getComponent<CameraComponent>();

        glm::mat4 view = camera.getView();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), commandBuffer.getWidth() / (float) commandBuffer.getHeight(), 0.1f, 10.0f);
        proj[1][1] *= -1; // TODO to camera

        auto entityView = entityRegistry->view<RenderComponent, ModelComponent, MaterialComponent, TransformComponent>();

        for(auto entity : entityView){
            
            auto transform = entityView.get<TransformComponent>(entity);

            glm::mat4 model = glm::scale(
                glm::rotate(glm::translate(glm::mat4(1.0f), transform.getPosition()),
                transform.getRotationAngle(), transform.getRotationVec()), transform.getScale()
            );

            if(!entityView.get<MaterialComponent>(entity).getDescriptorSet().size()){
                renderGraph->registerDescriptorSet(&entityView.get<MaterialComponent>(entity), {defaultTexture->getTextureView(), defaultTexture->getTextureSampler()});
            }

            commandBuffer
            .bind(entityView.get<MaterialComponent>(entity).getGraphicsPipeline())
            .bind(entityView.get<ModelComponent>(entity).getBuffers())
            .bind(entityView.get<MaterialComponent>(entity).getDescriptorSet())
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("model", model))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("view", view))
            .setUniform(entityView.get<MaterialComponent>(entity).uniform("proj", proj));

            for(auto uniform : entityView.get<MaterialComponent>(entity).getUserUniforms()){
                commandBuffer.setUniform(uniform);
            }

            commandBuffer.draw(entityView.get<ModelComponent>(entity).getCount());

        }

        if(getGameObject("Skybox")){
            GameObject* skybox = getGameObject("Skybox");

            if(!skybox->getComponent<MaterialComponent>().getDescriptorSet().size()){
                renderGraph->registerDescriptorSet(&skybox->getComponent<MaterialComponent>(), {defaultTexture->getTextureView(), defaultTexture->getTextureSampler()});
            }

            glm::mat4 stationaryView = glm::mat4(glm::mat3(view));  

            commandBuffer
            .bind(skybox->getComponent<MaterialComponent>().getGraphicsPipeline())
            .bind(skybox->getComponent<ModelComponent>().getBuffers())
            .bind(skybox->getComponent<MaterialComponent>().getDescriptorSet())
            .setUniform(skybox->getComponent<MaterialComponent>().uniform("proj", proj))
            .setUniform(skybox->getComponent<MaterialComponent>().uniform("view", stationaryView));

            commandBuffer.draw(skybox->getComponent<ModelComponent>().getCount());
        }

    }

    void loadScene(Vulkan& context){
        resourceManager.addDependency<ShaderProgram>(mainRenderpass);
        resourceManager.addDependency<ShaderProgram>(renderGraph);
        resourceManager.addDependency<Mesh>(context.getMemoryManager());
        resourceManager.addDependency<Texture>(context.getMemoryManager());

        defaultTexture = resourceManager.getResource<Texture>("./textures/NoTexture.jpg");

        setup();
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
        obj->addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/torus.glb"));
        obj->addComponent<TransformComponent>(
            glm::vec3(1.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.5f, 0.5f, 0.5f)
        );
        obj->addComponent<RenderComponent>();



        GameObject* skybox = spawnGameObject("Skybox");
        auto& mat = skybox->addComponent<MaterialComponent>(
            resourceManager.getResource<ShaderProgram>("./shaders/skybox.glsl")
        );
        mat.setTexture("Skybox", resourceManager.getResource<Texture>({ //TODO
            "./textures/skybox/right.jpg",
            "./textures/skybox/left.jpg",
            "./textures/skybox/top.jpg",
            "./textures/skybox/bottom.jpg",
            "./textures/skybox/front.jpg",
            "./textures/skybox/back.jpg"
        }));
        skybox->addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/cubemap.glb"));
    }

    void update(float deltaTime, Input& input){

        static float totalTime = 0.0;
        totalTime += deltaTime;

        if(!getGameObject("Object2")){
            GameObject* obj2 = spawnGameObject("Object2");
            obj2->addComponent<MaterialComponent>(resourceManager.getResource<ShaderProgram>("./shaders/nie.glsl"));
            obj2->addComponent<ModelComponent>(resourceManager.getResource<Mesh>("./models/cube.glb"));
            obj2->addComponent<TransformComponent>(
                glm::vec3(0.5f, 0.0f, 0.0f), 
                glm::vec3(0.0f, 0.0f, 0.0f), 
                glm::vec3(0.1f, 0.1f, 0.1f)
            );
            obj2->addComponent<RenderComponent>();
        }

        GameObject* obj = getGameObject("Object1");
        obj->getComponent<TransformComponent>().setRotation(glm::radians(45.0f) * totalTime, glm::vec3(0.0f, 0.0f, 1.0f));
        
        auto obj1Pos = obj->getComponent<TransformComponent>().getPosition();

        GameObject* obj2 = getGameObject("Object2");
        if(obj2){
            obj2->getComponent<TransformComponent>().setPosition({obj1Pos.x + cosf(-totalTime * 2.0f), obj1Pos.y + sinf(-totalTime * 2.0f), obj1Pos.z });
        }

        obj->getComponent<MaterialComponent>().setUniform("lightPos", obj2->getComponent<TransformComponent>().getPosition());
        
        static glm::vec3 cubeColor = {1.0f, 0.0f, 0.0f};

        ImGui::Begin("Color picker");

        ImGui::ColorPicker3("Cube color", glm::value_ptr(cubeColor));

        obj->getComponent<MaterialComponent>().setUniform("color", cubeColor);

        ImGui::End();

    }

};

}
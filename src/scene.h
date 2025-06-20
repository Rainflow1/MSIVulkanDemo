#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "json.h"
#include "resourceManager.h"
#include "vulkan/vulkanCore.h"
#include "input.h"
#include "gameobject.h"

#include <iostream>
#include <vector>
#include <type_traits>
#include <typeinfo>

namespace MSIVulkanDemo{


class Scene : public VulkanRendererI, public VulkanRenderGraphBuilderI, public JsonI{

private:
    std::map<std::string, std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<entt::registry> entityRegistry;
    std::shared_ptr<ImGuiInterface> gui;

    std::shared_ptr<VulkanRenderGraph> renderGraph;
    std::shared_ptr<VulkanRenderPass> mainRenderpass;

protected:
    std::shared_ptr<ResourceManager> resourceManager;

public:
    Scene(){
        entityRegistry = std::shared_ptr<entt::registry>(new entt::registry());
        resourceManager = std::shared_ptr<ResourceManager>(new ResourceManager());
        auto mainCamera = spawnGameObject("MainCamera");
        mainCamera->addComponent<TransformComponent>(glm::vec3(-2.0f, 0.0f, 1.0f));
        mainCamera->addComponent<CameraComponent>();
    }

    Scene(Scene& other) = delete;

    virtual ~Scene(){

        entityRegistry->clear();
    }

    virtual void setup() = 0;

    void updateScene(float deltaTime, Input& input){

        std::erase_if(gameObjects, [] (auto& kv){
            return kv.second->isRemoved();
        });

        auto camera = *entityRegistry->view<TransformComponent, CameraComponent>().begin();
        entityRegistry->get<CameraComponent>(camera).provideInput(deltaTime, input);

        static float resourceRefreshTime = 0.0f;

        if(resourceRefreshTime >= 3.0f){
            resourceManager->updateResources();
            resourceRefreshTime = 0.0f;
        }else{
            resourceRefreshTime += deltaTime;
        }

        ImGui::Begin("Scene objects", NULL, ImGuiWindowFlags_NoCollapse);

        static std::string selected = "";

        for(const auto& [name, gameObject] : gameObjects){
            ImGui::SetNextItemAllowOverlap();
            if(ImGui::Selectable(name.c_str(), selected == name)){
                selected = name;
            }
            ImGui::SameLine();
            if(ImGui::SmallButton(("X##" + name).c_str())){
                removeGameObject(name);
            }
        }

        ImGui::Separator();

        if(ImGui::Button("Create new object")){
            ImGui::OpenPopup("Create Object##Popup");
        }

        if (ImGui::BeginPopupModal("Create Object##Popup", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            ImGui::SetWindowPos({ImGui::GetIO().DisplaySize.x/2 - 150, ImGui::GetIO().DisplaySize.y/2 - 150});
            ImGui::SetWindowSize({300, 300});

            static std::string name = "\0";

            ImGui::InputText("Name", name.data(), name.capacity(), ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) -> int{
                if (data->EventFlag == ImGuiInputTextFlags_CallbackResize){
                    std::string* my_str = static_cast<std::string*>(data->UserData);
                    my_str->resize(data->BufSize);
                    data->Buf = my_str->data();
                }
                return 0;
            }, &name);

            if(ImGui::Button("Apply")){
                //TODO check if exists
                GameObject* obj = spawnGameObject(name);
                obj->addComponent<TransformComponent>();

                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if(ImGui::Button("Close")){
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGui::End();

        ImGui::Begin("Component inspector", NULL, ImGuiWindowFlags_NoCollapse);

        GameObject* selectedObject = getGameObject(selected);
        
        if(selectedObject){
            for(auto& component : selectedObject->getAllComponents()){
                component->guiDisplayInspector();
            }
        }

        if(selectedObject){
            ImGui::SeparatorText("");

            std::vector<GameObject::componentName> items = GameObject::getAllComponentsNames();

            items.erase(
                std::remove_if(
                    items.begin(), 
                    items.end(),
                    [=](GameObject::componentName const & p){
                        return selectedObject->hasComponent(p);
                    }
                ), 
                items.end()
            );

            if (ImGui::BeginCombo("##Add component", "Add component", ImGuiComboFlags_NoArrowButton)){
                static int selectedComponent = 0;
                for (int n = 0; n < items.size(); n++){
                    const bool is_selected = (selectedComponent == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected)){
                        selectedComponent = n;
                        selectedObject->addComponent(items[selectedComponent]);
                    }

                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

        }

        ImGui::End();

        return this->update(deltaTime, input);
    }

    virtual void update(float deltaTime, Input& input) = 0;

    void buildRenderGraph(std::shared_ptr<VulkanRenderGraph> renderGraph){
        /*
        renderGraph->addRenderPass("ShadowMap", 
            VulkanRenderGraph::AddRenderFunction([&](VulkanCommandBuffer& commandBuffer){
                this->render(commandBuffer);
            }),
            VulkanRenderGraph::AddDepthBuffer(),
            VulkanRenderGraph::SetRenderTargetOutput() // TODO
        );
        */
        renderGraph->addRenderPass("Main",
            //VulkanRenderGraph::SetRenderTargetInput("ShadowMap"), 
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

        auto materialView = entityRegistry->view<MaterialComponent>();

        for(auto material : materialView){
            if(!materialView.get<MaterialComponent>(material).getDescriptorSet().size()){
                renderGraph->registerDescriptorSet(&materialView.get<MaterialComponent>(material));
            }
        }

        auto camera = *entityRegistry->view<TransformComponent, CameraComponent>().begin();

        glm::mat4 view = entityRegistry->get<CameraComponent>(camera).getView();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), commandBuffer.getWidth() / (float) commandBuffer.getHeight(), 0.1f, 1000.0f);
        proj[1][1] *= -1; // TODO to camera

        auto entityView = entityRegistry->view<RenderComponent, ModelComponent, MaterialComponent, TransformComponent>(entt::exclude<SkyboxRendererComponent>);

        for(auto entity : entityView){
            
            auto transform = entityView.get<TransformComponent>(entity);

            glm::mat4 model = transform.getModel();

            entityView.get<MaterialComponent>(entity).setUniform("_viewPos", entityRegistry->get<TransformComponent>(camera).getPosition());
            entityView.get<MaterialComponent>(entity).setUniform("_model", model);
            entityView.get<MaterialComponent>(entity).setUniform("_view", view);
            entityView.get<MaterialComponent>(entity).setUniform("_proj", proj);

            commandBuffer
            .bind(entityView.get<MaterialComponent>(entity).getGraphicsPipeline())
            .bind(entityView.get<ModelComponent>(entity).getBuffers())
            .bind(entityView.get<MaterialComponent>(entity).getDescriptorSet())
            .setUniform(entityView.get<MaterialComponent>(entity).getUniforms());

            commandBuffer.draw(entityView.get<ModelComponent>(entity).getCount());

        }


        auto skybox = entityRegistry->view<SkyboxRendererComponent>();

        if(GameObject* skybox = getGameObject("Skybox"); skybox){
            
            if(!skybox->getComponent<MaterialComponent>().getDescriptorSet().size()){
                renderGraph->registerDescriptorSet(&skybox->getComponent<MaterialComponent>());
            }

            glm::mat4 stationaryView = glm::mat4(glm::mat3(view));  

            skybox->getComponent<MaterialComponent>().setUniform("_view", stationaryView);
            skybox->getComponent<MaterialComponent>().setUniform("_proj", proj);

            commandBuffer
            .bind(skybox->getComponent<MaterialComponent>().getGraphicsPipeline())
            .bind(skybox->getComponent<ModelComponent>().getBuffers())
            .bind(skybox->getComponent<MaterialComponent>().getDescriptorSet())
            .setUniform(skybox->getComponent<MaterialComponent>().getUniforms());

            commandBuffer.draw(skybox->getComponent<ModelComponent>().getCount());
        }

    }

    void loadScene(Vulkan& context){
        resourceManager->addDependency<ShaderProgram>(mainRenderpass);
        resourceManager->addDependency<ShaderProgram>(renderGraph);
        resourceManager->addDependency<Mesh>(context.getMemoryManager());
        resourceManager->addDependency<Texture>(context.getMemoryManager());

        setup();
    }

    void unloadScene(){
        return;
    }

    void loadGui(std::shared_ptr<ImGuiInterface> gui){
        this->gui = gui;
    }

    json saveToJson(){
        json scene;
        scene["objects"] = std::map<std::string, json>();
        for(auto& [name, gameobject] : gameObjects){

            scene["objects"].push_back({name, gameobject->saveToJson()});
        }
        
        return scene;
    }

    void loadFromJson(json scene){
        
        gameObjects.clear();

        for(auto& [name, gameObj] : scene["objects"].items()){
            auto* obj = spawnGameObject(name); 
            obj->loadFromJson(gameObj);
        }

        return;
    }

protected:
    GameObject* spawnGameObject(std::string objName){
        std::shared_ptr<GameObject> ptr = std::make_shared<GameObject>(entityRegistry, resourceManager);
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

    void removeGameObject(std::string objName){
        gameObjects.at(objName)->remove();
    }

private:


};


class SimpleScene : public Scene{

public:

    void setup(){
        
        GameObject* obj = spawnGameObject("Object1");
        obj->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/PBRLighting.glsl"));
        obj->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/torusHighpoly.glb"));
        obj->addComponent<TransformComponent>(
            glm::vec3(1.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f)
        );
        obj->addComponent<RenderComponent>();
        obj->getComponent<MaterialComponent>().setUniform<glm::vec3>("diffuse", {1.0f, 0.5f, 0.31f});
        obj->getComponent<MaterialComponent>().setUniform<glm::vec3>("specular", {0.5f, 0.5f, 0.5f});
        obj->getComponent<MaterialComponent>().setUniform<float>("shininess", 16.0f);
        obj->getComponent<MaterialComponent>().setTexture("albedoTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_albedo.png"));
        obj->getComponent<MaterialComponent>().setTexture("normTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_normal-ogl.png"));
        obj->getComponent<MaterialComponent>().setTexture("heightTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_height.png"));
        obj->getComponent<MaterialComponent>().setTexture("aoTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_ao.png"));
        obj->getComponent<MaterialComponent>().setTexture("metallicTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_metallic.png"));
        obj->getComponent<MaterialComponent>().setTexture("roughnessTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_roughness.png"));
        obj->getComponent<MaterialComponent>().setTexture("Skybox", resourceManager->getResource<Texture>({
            "./textures/skybox/right.jpg",
            "./textures/skybox/left.jpg",
            "./textures/skybox/top.jpg",
            "./textures/skybox/bottom.jpg",
            "./textures/skybox/front.jpg",
            "./textures/skybox/back.jpg"
        }));


        GameObject* skybox = spawnGameObject("Skybox");
        auto& mat = skybox->addComponent<MaterialComponent>(
            resourceManager->getResource<ShaderProgram>("./shaders/skybox.glsl")
        );
        mat.setTexture("Skybox", resourceManager->getResource<Texture>({
            "./textures/skybox/right.jpg",
            "./textures/skybox/left.jpg",
            "./textures/skybox/top.jpg",
            "./textures/skybox/bottom.jpg",
            "./textures/skybox/front.jpg",
            "./textures/skybox/back.jpg"
        }));
        skybox->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/cubemap.glb"));
    }

    void update(float deltaTime, Input& input){

        static float totalTime = 0.0;
        totalTime += deltaTime;

        if(!getGameObject("Object2")){
            GameObject* obj2 = spawnGameObject("Object2");
            obj2->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/nie.glsl"));
            obj2->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/cubeuv.glb"));
            obj2->addComponent<TransformComponent>(
                glm::vec3(0.5f, 0.0f, 0.0f), 
                glm::vec3(0.0f, 0.0f, 0.0f), 
                glm::vec3(0.1f, 0.1f, 0.1f)
            );
            obj2->addComponent<RenderComponent>();
        }

        GameObject* obj = getGameObject("Object1");
        obj->getComponent<TransformComponent>().setRotation(glm::vec3(0.0f, 0.0f, glm::radians(45.0f) * totalTime));
        
        auto obj1Pos = obj->getComponent<TransformComponent>().getPosition();

        GameObject* obj2 = getGameObject("Object2");
        if(obj2){
            obj2->getComponent<TransformComponent>().setPosition({obj1Pos.x + cosf(-totalTime * 2.0f), obj1Pos.y + sinf(-totalTime * 2.0f), obj1Pos.z });
        }

        obj->getComponent<MaterialComponent>().setUniform("lightPos", obj2->getComponent<TransformComponent>().getPosition());

    }

};


class DefaultScene : public Scene{

public:

    void setup(){
        
        
        
    }

    void update(float deltaTime, Input& input){

        

    }

};


}
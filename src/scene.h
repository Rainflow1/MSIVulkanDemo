#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "json.h"
#include "resourceManager.h"
#include "vulkan/vulkanCore.h"
#include "input.h"
#include "gameobjectManagerI.h"
#include "gameobject.h"
#include "scriptManager.h"

#include <iostream>
#include <vector>
#include <type_traits>
#include <typeinfo>

namespace MSIVulkanDemo{


class Scene : std::enable_shared_from_this<GameobjectManagerI>, public GameobjectManagerI, public VulkanRendererI, public VulkanRenderGraphBuilderI, public JsonI{

private:
    std::map<std::string, std::shared_ptr<GameObject>> gameObjects;
    std::shared_ptr<entt::registry> entityRegistry;
    std::shared_ptr<ImGuiInterface> gui;

    std::shared_ptr<VulkanRenderGraph> renderGraph;
    std::shared_ptr<VulkanRenderPass> mainRenderpass;

    std::shared_ptr<ScriptManager> scriptManager;

protected:
    std::shared_ptr<ResourceManager> resourceManager;

public:
    Scene(){
        entityRegistry = std::shared_ptr<entt::registry>(new entt::registry());
        resourceManager = std::shared_ptr<ResourceManager>(new ResourceManager());
        auto mainCamera = spawnGameObject("MainCamera");
        mainCamera->addComponent<TransformComponent>(glm::vec3(-2.0f, 0.0f, 1.0f));
        mainCamera->addComponent<CameraComponent>();
        scriptManager = std::shared_ptr<ScriptManager>(new ScriptManager());
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
                std::shared_ptr<GameObject> obj = spawnGameObject(name);
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

        std::shared_ptr<GameObject> selectedObject = getGameObject(selected);
        
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

        auto scriptView = entityRegistry->view<ScriptComponent>();

        for(auto script : scriptView){
            scriptView.get<ScriptComponent>(script).execUpdate(deltaTime);
        }


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
/*
        renderGraph->addRenderPass("Postprocess",
            //VulkanRenderGraph::SetRenderTargetInput("ShadowMap"), 
            VulkanRenderGraph::AddRenderFunction([&](VulkanCommandBuffer& commandBuffer){
                this->render(commandBuffer);
            }),
            VulkanRenderGraph::AddDepthBuffer()
        );
*/
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

        if(std::shared_ptr<GameObject> skybox = getGameObject("Skybox"); skybox){
            
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
        resourceManager->addDependency<Script>(scriptManager);

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
            auto obj = spawnGameObject(name); 
            obj->loadFromJson(gameObj);
        }

        return;
    }

public:
    std::shared_ptr<GameObject> spawnGameObject(std::string objName){
        std::shared_ptr<GameObject> ptr = std::make_shared<GameObject>(entityRegistry, resourceManager, this);
        gameObjects.insert({objName, ptr});
        return ptr;
    }

    std::shared_ptr<GameObject> getGameObject(std::string objName){     
        if(gameObjects.find(objName) == gameObjects.end()){
            return nullptr;
        }
        return gameObjects[objName];
    }

    std::vector<std::pair<std::string, std::shared_ptr<GameObject>>> getAllGameObjects(){
        std::vector<std::pair<std::string, std::shared_ptr<GameObject>>> objects;

        for(const auto [name, objPtr] : gameObjects){
            objects.push_back({name, objPtr});
        }

        return objects;
    }

    void removeGameObject(std::string objName){
        gameObjects.at(objName)->remove();
    }

    std::string getGameObjectName(std::shared_ptr<GameObject> obj){
        std::string name = "Null";
        
        for(const auto [objName, objPtr] : gameObjects){
            if(objPtr == obj){
                name = objName;
            }
        }

        return name;
    }

private:


};


class SimpleScene : public Scene{

public:

    void setup(){
        
        auto skyboxTex = resourceManager->getResource<Texture>({
            "./textures/mrzezinoSkybox/px.png",
            "./textures/mrzezinoSkybox/nx.png",
            "./textures/mrzezinoSkybox/py.png",
            "./textures/mrzezinoSkybox/ny.png",
            "./textures/mrzezinoSkybox/pz.png",
            "./textures/mrzezinoSkybox/nz.png"
        });

        std::shared_ptr<GameObject> lightSrc = spawnGameObject("light source");
        lightSrc->addComponent<TransformComponent>(
            glm::vec3(1.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f)
        );
        lightSrc->addComponent<ScriptComponent>(resourceManager->getResource<Script>("./scripts/circle.py"));
        //script.getScript("Circle")->setProperty<glm::vec3>("offset", {0.0f, 5.0f, 0.0f});

        /*
        for(uint32_t i = 0; i < 5; i++){
            for(uint32_t j = 0; j < 5; j++){
                GameObject* pbr = spawnGameObject(std::format("pbr_{}_{}", i, j));
                pbr->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/PBRLighting.glsl"));
                pbr->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/sphereHighpoly.glb"));
                pbr->addComponent<TransformComponent>(
                    glm::vec3(1.0f * i, 1.0f * j, 0.0f), 
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.25f, 0.25f, 0.25f)
                );
                pbr->addComponent<RenderComponent>();
                pbr->getComponent<MaterialComponent>().setUniform<glm::vec3>("inAlbedo", {1.0f, 0.5f, 0.0f});
                pbr->getComponent<MaterialComponent>().setUniform<float>("inRoughness", 0.2f * i + 0.1f);
                pbr->getComponent<MaterialComponent>().setUniform<float>("inMetallic", 0.2f * j + 0.1f);
                pbr->getComponent<MaterialComponent>().setUniform<float>("inReflectance", 0.2f * j + 0.1f);

                pbr->getComponent<MaterialComponent>().setTexture("Skybox", skyboxTex);
            }
        }
        */




        auto pbr = spawnGameObject("pbr");
        pbr->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/PBRLighting.glsl"));
        pbr->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/sphereHighpoly.glb"));
        pbr->addComponent<TransformComponent>(
            glm::vec3(0.0f, 0.0f, 2.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.25f, 0.25f, 0.25f)
        );
        pbr->addComponent<RenderComponent>();
        pbr->getComponent<MaterialComponent>().setUniform<glm::vec3>("inAlbedo", {0.0f, 1.0f, 0.2f});
        pbr->getComponent<MaterialComponent>().setUniform<float>("inRoughness", 0.5f);
        pbr->getComponent<MaterialComponent>().setUniform<float>("inMetallic", 0.5f);
        pbr->getComponent<MaterialComponent>().setUniform<float>("inReflectance", 0.75f);

        pbr->getComponent<MaterialComponent>().setTexture("Skybox", skyboxTex);


        auto pbrTex = spawnGameObject("pbrTex");
        pbrTex->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/PBRLightingTexture.glsl"));
        pbrTex->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/torusHighpoly.glb"));
        pbrTex->addComponent<TransformComponent>(
            glm::vec3(0.0f, 0.0f, 1.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.5f, 0.5f, 0.5f)
        );
        pbrTex->addComponent<RenderComponent>();
        
        pbrTex->getComponent<MaterialComponent>().setTexture("albedoTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_albedo.png"));
        pbrTex->getComponent<MaterialComponent>().setTexture("normTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_normal-ogl.png"));
        pbrTex->getComponent<MaterialComponent>().setTexture("heightTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_height.png"));
        pbrTex->getComponent<MaterialComponent>().setTexture("aoTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_ao.png"));
        pbrTex->getComponent<MaterialComponent>().setTexture("metallicTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_metallic.png"));
        pbrTex->getComponent<MaterialComponent>().setTexture("roughnessTex", resourceManager->getResource<Texture>("./textures/fancy-scaled-gold-bl/fancy-scaled-gold_roughness.png"));

        pbrTex->getComponent<MaterialComponent>().setTexture("Skybox", skyboxTex);

        auto phong = spawnGameObject("phong");
        phong->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/phongLighting.glsl"));
        phong->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/cube.glb"));
        phong->addComponent<TransformComponent>(
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.25f, 0.25f, 0.25f)
        );
        phong->addComponent<RenderComponent>();

        phong->getComponent<MaterialComponent>().setUniform<glm::vec3>("ambient", {1.0f, 0.0f, 0.31f});
        phong->getComponent<MaterialComponent>().setUniform<glm::vec3>("diffuse", {1.0f, 1.0f, 0.31f});
        phong->getComponent<MaterialComponent>().setUniform<glm::vec3>("specular", {0.0f, 0.5f, 0.5f});
        phong->getComponent<MaterialComponent>().setUniform<float>("shininess", 32.0f);

        //obj->addComponent<ScriptComponent>(resourceManager->getResource<Script>("./scripts/bindToUniform.py"));
        //script.addScript(resourceManager->getResource<Script>("./scripts/bindToUniform.py"));
        //script.getScript("BindToUniform")->setProperty<PythonBindings::PyString>("uniformName", PythonBindings::PyString("lightPos"));
        //script.getScript("BindToUniform")->setProperty<PythonBindings::ObjectRef>("bindedObj", PythonBindings::ObjectRef());

        
        auto obj2 = spawnGameObject("Object2");
        obj2->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/default.glsl"));
        obj2->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/cubeuv.glb"));
        obj2->addComponent<TransformComponent>(
            glm::vec3(0.5f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f), 
            glm::vec3(0.1f, 0.1f, 0.1f)
        );
        obj2->addComponent<RenderComponent>();
        //obj2->addComponent<ScriptComponent>(resourceManager->getResource<Script>("./scripts/circle.py"));
        


        auto gouraudPhong = spawnGameObject("gouraudPhong");
        gouraudPhong->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/gouraudPhongLighting.glsl"));
        gouraudPhong->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/sphereHighpoly.glb"));
        gouraudPhong->addComponent<TransformComponent>(
            glm::vec3(2.0f, 0.0f, 0.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.25f, 0.25f, 0.25f)
        );
        gouraudPhong->addComponent<RenderComponent>();

        gouraudPhong->getComponent<MaterialComponent>().setUniform<glm::vec3>("ambient", {1.0f, 0.5f, 0.5f});
        gouraudPhong->getComponent<MaterialComponent>().setUniform<glm::vec3>("diffuse", {0.0f, 0.0f, 0.1f});
        gouraudPhong->getComponent<MaterialComponent>().setUniform<glm::vec3>("specular", {1.0f, 1.0f, 0.5f});
        gouraudPhong->getComponent<MaterialComponent>().setUniform<float>("shininess", 64.0f);


        auto phongTex = spawnGameObject("phongTex");
        phongTex->addComponent<MaterialComponent>(resourceManager->getResource<ShaderProgram>("./shaders/phongLightingTexture.glsl"));
        phongTex->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/cubeuv.glb"));
        phongTex->addComponent<TransformComponent>(
            glm::vec3(2.0f, 0.0f, 1.0f), 
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.25f, 0.25f, 0.25f)
        );
        phongTex->addComponent<RenderComponent>();

        phongTex->getComponent<MaterialComponent>().setUniform<glm::vec3>("ambient", {1.0f, 1.0f, 1.0f});
        phongTex->getComponent<MaterialComponent>().setUniform<glm::vec3>("diffuse", {1.0f, 0.5f, 0.31f});
        phongTex->getComponent<MaterialComponent>().setUniform<glm::vec3>("specular", {1.0f, 0.0f, 1.0f});
        phongTex->getComponent<MaterialComponent>().setUniform<float>("shininess", 32.0f);
        phongTex->getComponent<MaterialComponent>().setTexture("tex", resourceManager->getResource<Texture>("./textures/Tiles.jpg"));
        





        std::shared_ptr<GameObject> skybox = spawnGameObject("Skybox");
        auto& mat = skybox->addComponent<MaterialComponent>(
            resourceManager->getResource<ShaderProgram>("./shaders/skybox.glsl")
        );
        mat.setTexture("Skybox", skyboxTex);
        /*
        mat.setTexture("Skybox", resourceManager->getResource<Texture>({
            "./textures/skybox/right.jpg",
            "./textures/skybox/left.jpg",
            "./textures/skybox/top.jpg",
            "./textures/skybox/bottom.jpg",
            "./textures/skybox/front.jpg",
            "./textures/skybox/back.jpg"
        }));
        */
        skybox->addComponent<ModelComponent>(resourceManager->getResource<Mesh>("./models/cubemap.glb"));
    }

    void update(float deltaTime, Input& input){
/*
        auto light = getGameObject("light source");

        if(light){
            auto lightPos = light->getComponent<TransformComponent>().getPosition();

            for(const auto& [name, obj] : getAllGameObjects()){
                if(obj->hasComponent<MaterialComponent>()){
                    auto& mat = obj->getComponent<MaterialComponent>();
                    if(mat.hasUniform("lightPos")){
                        mat.setUniform<glm::vec3>("lightPos", lightPos);
                    }
                }
            }
        }        
*/
/*
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
*/
    }

};


class DefaultScene : public Scene{

public:

    void setup(){
        

        
    }

    void update(float deltaTime, Input& input){
/*
        auto light = getGameObject("light source");

        if(light){
            auto lightPos = light->getComponent<TransformComponent>().getPosition();

            for(const auto& [name, obj] : getAllGameObjects()){
                if(obj->hasComponent<MaterialComponent>()){
                    auto& mat = obj->getComponent<MaterialComponent>();
                    if(mat.hasUniform("lightPos")){
                        mat.setUniform<glm::vec3>("lightPos", lightPos);
                    }
                }
            }
        }  
*/
    }

};


}
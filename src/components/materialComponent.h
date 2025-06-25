#pragma once

#include "../fileDialog.h"

#include "../component_decl.h"
#include "../resources/shaderProgram.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class MaterialComponent : public Component, public VulkanDescriptorSetOwner{
private:
    std::string shaderToCompile;
    std::shared_ptr<ShaderProgram> shaderProgram; 
    std::vector<std::shared_ptr<VulkanDescriptorSet>> descriptorSet;

    std::map<std::string, std::vector<float>> floatUniforms;
    std::map<std::string, std::shared_ptr<Texture>> textures;

public:
    MaterialComponent(std::shared_ptr<ResourceManager> resMgr): Component(resMgr), shaderProgram(resMgr->getResource<ShaderProgram>("./shaders/default.glsl")){

    }

    MaterialComponent(std::shared_ptr<ShaderProgram> shaderProgram) : shaderProgram(shaderProgram){

    }

    ~MaterialComponent(){
        
    }

    void afterResourceManager() override{
        updateUniforms();
    };

    std::shared_ptr<VulkanGraphicsPipeline> getGraphicsPipeline(){
        return shaderProgram->getGraphicsPipeline();
    }

    void updateUniforms(){
        for(auto attrib : shaderProgram->getGraphicsPipeline()->getUniformData().getAttributes()){
            if(attrib.size > 0){
                std::vector<float> val;
                for(uint32_t i = 0; i < attrib.componentCount; i++){
                    val.push_back(0.0f);
                }
                floatUniforms.insert({attrib.name, val});
            }else{
                if(attrib.componentCount == 6){
                    textures.insert({attrib.name, resourceManager->getResource<Texture>({"./textures/NoTexture.jpg", "./textures/NoTexture.jpg", "./textures/NoTexture.jpg", "./textures/NoTexture.jpg", "./textures/NoTexture.jpg", "./textures/NoTexture.jpg"})});
                }else{
                    textures.insert({attrib.name, resourceManager->getResource<Texture>("./textures/NoTexture.jpg")});
                }
            }
        }
    }

    bool hasUniform(std::string name){
        return shaderProgram->getGraphicsPipeline()->getUniformData().contains(name);
    }

    template<typename t>
    typename std::enable_if<(sizeof(t)%sizeof(float) == 0)>::type
    setUniform(std::string name, t val){
        std::vector<float> arr(sizeof(t)/sizeof(float));
        
        for(int i = 0; i < sizeof(t)/sizeof(float); i++){
            arr[i] = reinterpret_cast<float*>(&val)[i];
        }

        if(floatUniforms.find(name) != floatUniforms.end()){
            floatUniforms[name] = arr;
            return;
        }
        
        //std::cout << "Uniform non existant " << name << std::endl;
        //floatUniforms.insert({name, arr});
    }

    void setTexture(std::string name, std::shared_ptr<Texture> texture){
        if(textures.contains(name)){
            textures.at(name) = texture;
        }else{
            textures.insert({name, texture});
        }

        updateDescriptorSet();
    }

    std::vector<std::pair<size_t, std::vector<float>>> getUniforms(){
        std::vector<std::pair<size_t, std::vector<float>>> uni;

        for(auto const& [key, val] : floatUniforms){
            if(shaderProgram->getGraphicsPipeline()->getUniformData().contains(key)){
                uni.push_back(std::pair<size_t, std::vector<float>>(shaderProgram->getGraphicsPipeline()->getUniformData().getOffset(key), val));
            }
        }

        return uni;
    }

    void setDescriptorSet(std::vector<std::shared_ptr<VulkanDescriptorSet>> sets){
        descriptorSet = sets;

        updateUniforms();
        updateDescriptorSet();
        
    }

    void updateDescriptorSet(){
        for(const auto& [name, texture] : textures){
            for(auto& set : descriptorSet){
                set->setTexture(name, texture->getTextureView(), texture->getTextureSampler());
            }
        }
        for(auto& set : descriptorSet){
            set->writeDescriptorSet(shaderProgram->getGraphicsPipeline()->getUniformData());
        }
    }

    void clearUniformsAndDescriptorSet(){
        descriptorSet.clear();
        floatUniforms.clear();
        textures.clear();
    }

    std::vector<std::shared_ptr<VulkanDescriptorSet>> getDescriptorSet(){
        return descriptorSet;
    }

    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Material")){

            ImGui::Text("Shader: ");
            ImGui::SameLine();
            if(ImGui::SmallButton((shaderProgram->getPath()).c_str())){

                std::shared_ptr<ShaderProgram> newShaderProgram;

                try{
                    newShaderProgram = resourceManager->getResource<ShaderProgram>(std::filesystem::relative(FileDialog::fileDialog().getPath()).string());
                }catch(std::exception ex){
                    std::cout << "Can`t compile shader" << std::endl;
                }

                if(newShaderProgram){
                    shaderProgram = newShaderProgram;
                    clearUniformsAndDescriptorSet();
                }
                
            }



            const float step = 0.01f;
            ImGui::SeparatorText("Uniforms: ");

            for(auto& [name, uniform] : floatUniforms){
                ImGui::Separator();          
                switch(uniform.size()){

                case 16:
                    ImGui::DragFloat4(name.c_str(), floatUniforms.at(name).data(), step);
                    ImGui::DragFloat4(("##" + name+"1").c_str(), floatUniforms.at(name).data()+4, step);
                    ImGui::DragFloat4(("##" + name+"2").c_str(), floatUniforms.at(name).data()+8, step);
                    ImGui::DragFloat4(("##" + name+"3").c_str(), floatUniforms.at(name).data()+12, step);
                    break;

                case 9:
                    ImGui::DragFloat3(name.c_str(), floatUniforms.at(name).data(), step);
                    ImGui::DragFloat3(("##" + name+"1").c_str(), floatUniforms.at(name).data()+3, step);
                    ImGui::DragFloat3(("##" + name+"2").c_str(), floatUniforms.at(name).data()+6, step);
                    break;

                case 4:
                    ImGui::DragFloat4(name.c_str(), floatUniforms.at(name).data(), step);
                    break;

                case 3:
                    ImGui::DragFloat3(name.c_str(), floatUniforms.at(name).data(), step);
                    ImGui::SameLine();
                    if(ImGui::SmallButton(("C##" + name).c_str())){
                        ImGui::OpenPopup(("ColorPicker##Popup" + name).c_str());
                    }

                    if (ImGui::BeginPopupModal(("ColorPicker##Popup" + name).c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
                        ImGui::SetWindowPos({ImGui::GetIO().DisplaySize.x/2 - 150, ImGui::GetIO().DisplaySize.y/2 - 150});
                        ImGui::SetWindowSize({300, 300});

                        static glm::vec3 color;

                        ImGui::ColorPicker3("Color", glm::value_ptr(color));

                        if(ImGui::Button("Apply")){
                            floatUniforms.at(name)[0] = color.x;
                            floatUniforms.at(name)[1] = color.y;
                            floatUniforms.at(name)[2] = color.z;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if(ImGui::Button("Close")){
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                    break;

                case 2:
                    ImGui::DragFloat2(name.c_str(), floatUniforms.at(name).data(), step);
                    break;

                case 1:
                    ImGui::DragFloat(name.c_str(), floatUniforms.at(name).data(), step);
                    break;
                
                default:
                    break;
                }
            }

            

            ImGui::SeparatorText("Textures: ");

            for(auto& [name, tex] : textures){           
                
                if(shaderProgram->getGraphicsPipeline()->getUniformData().getAttribute(name).componentCount == 6){

                    const std::array<std::string, 6> names = {"right", "left", "top", "bottom", "front", "back"};

                    std::vector<std::string> filePaths = tex->getPaths(); // FIXME

                    ImGui::Text((name + ": ").c_str());
                    ImGui::Indent(10.0f);

                    for(uint32_t i = 0; i < 6; i++){                           
                        ImGui::Text((names[i] + ": ").c_str());
                        ImGui::SameLine();
                        if(ImGui::SmallButton((filePaths[i] + "##" + name + names[i]).c_str())){ 
                            auto filePath = std::filesystem::relative(FileDialog::fileDialog().getPath()).string();

                            if(!filePath.empty()){
                                filePaths[i] = filePath;
                            }
                        }
                    }

                    if(ImGui::Button(("Confirm##" + name).c_str())){
                        try{
                            std::shared_ptr<Texture> newTex = resourceManager->getResource<Texture>(filePaths);
                            setTexture(name, newTex);
                        }catch(std::exception e){
                            std::cout << "Cant load texture" << std::endl;
                        }
                    }

                    ImGui::Unindent(10.0f);

                }else{
                    ImGui::Text((name + ": ").c_str());
                    ImGui::SameLine();
                    if(ImGui::SmallButton((tex->getPath() + "##" + name).c_str())){
                        auto filePath = std::filesystem::relative(FileDialog::fileDialog().getPath()).string();

                        if(!filePath.empty()){
                            setTexture(name, resourceManager->getResource<Texture>(filePath));
                        }

                    }

                }

            }

        }
    }


    json saveToJson(){
        json component;
        
        component["shader"] = shaderProgram->getPath();
        component["uniforms"] = floatUniforms;
        component["textures"] = std::map<std::string, std::string>();

        for(auto& [name, tex] : textures){
            component["textures"].push_back({name, tex->getPaths()});
        }

        return component;
    }

    void loadFromJson(json component){

        shaderProgram = resourceManager->getResource<ShaderProgram>(component["shader"].get<std::string>());
        clearUniformsAndDescriptorSet();

        for(auto& [name, val] : component["uniforms"].items()){
            floatUniforms.insert({name, val.get<std::vector<float>>()});
        }

        for(auto& [name, tex] : component["textures"].items()){
            if(tex.size() > 1){
                setTexture(name, resourceManager->getResource<Texture>(tex.get<std::vector<std::string>>()));
            }else{
                setTexture(name, resourceManager->getResource<Texture>(tex[0].get<std::string>()));
            }
        }

        return;
    }

};



}
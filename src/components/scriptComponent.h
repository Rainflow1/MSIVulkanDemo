#pragma once

#include "../component.h"
#include "../resources/script.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class ScriptComponent : public Component{
private:
    std::vector<std::shared_ptr<Script>> scripts;

public:
    ScriptComponent(){
        
    }

    ScriptComponent(std::shared_ptr<ResourceManager> resMgr) : Component(resMgr){
        
    }

    ScriptComponent(std::shared_ptr<Script> script){
        addScript(script);
    }

    void addScript(std::shared_ptr<Script> script){
        scripts.push_back(script);
    }

    std::shared_ptr<Script> getScript(std::string name){
        for(auto& script : scripts){
            if(script->getName() == name){
                return script;
            }
        }
        return nullptr;
    }

    void execUpdate(float deltatime){

        for(auto script : scripts){
            if(!script->isStarted()){
                script->start(owner.lock());
            }

            script->update(deltatime);
        }
    }


    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Script")){
            
            ImGui::Text("Scripts: ");

            const float step = 0.1f;

            for(const auto script : scripts){
                ImGui::SeparatorText((script->getName()).c_str());

                ImGui::Text("Propertiers: ");
                for(const auto [name, property] : script->getProperties()){
                    ImGui::Separator();
                    if(property.type() == typeid(glm::vec3*)){
                        ImGui::DragFloat3(name.c_str(), glm::value_ptr(*std::any_cast<glm::vec3*>(property)), step);
                    }
                    
                    if(property.type() == typeid(std::string*)){

                        std::string* strBuf = std::any_cast<std::string*>(property);

                        ImGui::InputText(name.c_str(), strBuf->data(), strBuf->capacity(), ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) -> int{
                            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize){
                                std::string* my_str = static_cast<std::string*>(data->UserData);
                                my_str->resize(data->BufSize);
                                data->Buf = my_str->data();
                            }
                            return 0;
                        }, strBuf);

                    }

                    if(property.type() == typeid(PythonBindings::ObjectRef*)){
                        ImGui::Text((name + ": ").c_str());
                        ImGui::SameLine();

                        PythonBindings::ObjectRef* objRef = std::any_cast<PythonBindings::ObjectRef*>(property);

                        if(ImGui::BeginCombo(("##Select object" + name).c_str(), gameobjectManager->getGameObjectName(objRef->getReference()).c_str(), ImGuiComboFlags_NoArrowButton)){
                            
                            if (ImGui::Selectable("Null", objRef->getReference() == nullptr)){
                                objRef->setReference(nullptr);
                            }

                            for(const auto [objName, obj] : gameobjectManager->getAllGameObjects()){
                                
                                if(obj == owner.lock()){
                                    continue;
                                }
                                
                                const bool is_selected = (objRef->getReference() == obj);

                                if (ImGui::Selectable(objName.c_str(), is_selected)){
                                    objRef->setReference(obj);
                                }

                                if (is_selected)
                                    ImGui::SetItemDefaultFocus();
                            }
                            
                            ImGui::EndCombo();
                        }

                    }
                }

            }

            ImGui::Separator();
            if(ImGui::SmallButton("Add script")){
                auto filePath = std::filesystem::relative(FileDialog::fileDialog().getPath()).string();

                if(!filePath.empty()){
                    scripts.push_back(resourceManager->getResource<Script>(filePath));
                }
            }

        }
    }


    json saveToJson(){
        json component;

        for(const auto& script : scripts){

            std::map<std::string, json> content;

            for(const auto& [name, prop] : script->getProperties()){
                
                if(prop.type() == typeid(glm::vec3*)){
                    glm::vec3 val = *std::any_cast<glm::vec3*>(prop);
                    content.insert({name, {{"type", "vec3"}, {"value", {val.x, val.y, val.z}}}});
                }
                
                if(prop.type() == typeid(std::string*)){
                    std::string val = *std::any_cast<std::string*>(prop);
                    content.insert({name, {{"type", "string"}, {"value", val}}});
                }

                if(prop.type() == typeid(PythonBindings::ObjectRef*)){
                    PythonBindings::ObjectRef* objRef = std::any_cast<PythonBindings::ObjectRef*>(prop);
                    content.insert({name, {{"type", "object"}, {"value", gameobjectManager->getGameObjectName(objRef->getReference())}}});
                }
            }

            component.emplace(script->getPath(), content);
        }
        
        return component;
    }

    void loadFromJson(json obj){

        for(const auto& [key, val] : obj.items()){
            auto script = resourceManager->getResource<Script>(key);
            addScript(script);
            for(const auto& [k, v] : val.items()){
                if(v["type"] == "vec3"){
                    auto vv = v["value"].get<std::vector<float>>();
                    script->setProperty<glm::vec3>(k, glm::vec3(vv[0], vv[1], vv[2]));
                }
                // TODO
            }
        }

        return;
    }


};



}
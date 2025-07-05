#pragma once

#include "../component.h"
#include "../resources/script.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class ScriptComponent : public Component{
private:
    std::vector<std::shared_ptr<Script>> scriptFiles;

    std::map<std::string, py::object> scripts;

public:
    ScriptComponent(ComponentParams& params) : Component(params){
        
    }

    ScriptComponent(ComponentParams& params, std::shared_ptr<Script> script) : Component(params){
        addScript(script);
    }

    void addScript(std::shared_ptr<Script> script){
        scriptFiles.push_back(script);
        
        for(const auto& [name, behaviour] : script->getBehaviours()){
            try{
                scripts.insert({name, behaviour()});
                auto scr = scripts.at(name);

                auto objCast = scr.cast<PythonBindings::Behaviour*>();
                if(!objCast->isStarted){
                    objCast->isStarted = true;
                    objCast->gameObject = owner.lock();
                    scr.attr("start")();

                    for(const auto handle : scr.attr("__dict__")){
                        py::object objProp = scr.attr(handle);
                        for(auto& prop : objCast->propertiesQueue){
                            if(objProp.is(prop)){
                                objCast->properties.insert({handle.cast<std::string>(), prop});
                                break;
                            }
                        }
                    }
                }

            }catch(py::error_already_set e){
                std::cout << "Python error in " << name << ": " << std::endl;
                std::cout << e.what() << std::endl;
            }
        }
    }
/*
    std::shared_ptr<Script> getScript(std::string name){
        for(const auto& scriptFile : scriptFiles){
            for(const auto& [behName, script] : scriptFile->getBehaviours()){
                if(behName == name){
                    return script;
                }
            }
        }
        return nullptr;
    }
*/
    void execUpdate(float deltatime){

        for(auto [name, script] : scripts){

            try{
                
                script.attr("update")(deltatime);

            }catch(py::error_already_set e){
                std::cout << "Python error in " << name << ": " << std::endl;
                std::cout << e.what() << std::endl;
            }
        }
    }


    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Script")){
            
            ImGui::Text("Scripts: ");

            const float step = 0.1f;

            for(const auto& [name, script] : scripts){

                ImGui::SeparatorText((name).c_str());

                ImGui::Text("Properties: ");
                for(const auto [propName, property] : script.cast<PythonBindings::Behaviour*>()->getProperties()){
                    ImGui::Separator();

                    if(py::type::of(property) == py::type::of<glm::vec3>()){
                        ImGui::DragFloat3(propName.c_str(), glm::value_ptr(*property.cast<glm::vec3*>()), step);
                    }
                    
                    if(py::type::of(property) == py::type::of<PythonBindings::PyString>()){

                        PythonBindings::PyString* strBuf = static_cast<PythonBindings::PyString*>(property.cast<PythonBindings::PyString*>());

                        ImGui::InputText(propName.c_str(), strBuf->str().data(), strBuf->str().capacity(), ImGuiInputTextFlags_CallbackResize, [](ImGuiInputTextCallbackData* data) -> int{
                            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize){
                                std::string* my_str = static_cast<std::string*>(data->UserData);
                                my_str->resize(data->BufSize);
                                data->Buf = my_str->data();
                            }
                            return 0;
                        }, strBuf);

                    }

                    if(py::type::of(property) == py::type::of<PythonBindings::ObjectRef>()){
                        ImGui::Text((propName + ": ").c_str());
                        ImGui::SameLine();

                        PythonBindings::ObjectRef* objRef = property.cast<PythonBindings::ObjectRef*>();

                        if(ImGui::BeginCombo(("##Select object" + propName).c_str(), gameobjectManager->getGameObjectName(objRef->getReference()).c_str(), ImGuiComboFlags_NoArrowButton)){
                            
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
                auto filePath = FileDialog::fileDialog().getPath();

                if(!filePath.empty()){
                    addScript(resourceManager->getResource<Script>(filePath));
                }
            }

        }
    }


    json saveToJson(){
        json component;

        for(const auto& [name, script] : scripts){
            
            std::map<std::string, json> content;

            auto objCast = script.cast<PythonBindings::Behaviour*>();

            for(const auto& [propName, property] : objCast->getProperties()){

                if(py::type::of(property) == py::type::of<glm::vec3>()){
                    glm::vec3 val = *property.cast<glm::vec3*>();
                    content.insert({propName, {{"type", "vec3"}, {"value", {val.x, val.y, val.z}}}});
                }

                if(py::type::of(property) == py::type::of<PythonBindings::PyString>()){
                    std::string val = *property.cast<PythonBindings::PyString*>();
                    content.insert({propName, {{"type", "string"}, {"value", val}}});
                }

                if(py::type::of(property) == py::type::of<PythonBindings::ObjectRef>()){
                    PythonBindings::ObjectRef* objRef = property.cast<PythonBindings::ObjectRef*>();
                    content.insert({propName, {{"type", "object"}, {"value", gameobjectManager->getGameObjectName(objRef->getReference())}}});
                }
            }

            component["properties"][name] = content;
        }

        for(const auto& scriptFile : scriptFiles){
            component["scripts"].push_back(scriptFile->getPath());
        }
        
        return component;
    }

    void loadFromJson(json obj){

        for(std::string path : obj["scripts"]){
            auto script = resourceManager->getResource<Script>(path);
            addScript(script);
        }

        for(const auto& [scriptName, properties] : obj["properties"].items()){
            auto script = scripts.at(scriptName);
            for(const auto& [propName, property] : properties.items()){

                if(property["type"] == "vec3"){
                    auto val = property["value"].get<std::vector<float>>();
                    script.cast<PythonBindings::Behaviour*>()->setProperty<glm::vec3>(propName, glm::vec3(val[0], val[1], val[2]));
                }
                
                if(property["type"] == "string"){
                    auto val = property["value"].get<std::string>();
                    script.cast<PythonBindings::Behaviour*>()->setProperty<PythonBindings::PyString>(propName, PythonBindings::PyString(val));
                }

                if(property["type"] == "object"){
                    auto val = property["value"].get<std::string>(); // FIXME load order dependent (getGameObject returns null)
                    script.cast<PythonBindings::Behaviour*>()->setProperty<PythonBindings::ObjectRef>(propName, PythonBindings::ObjectRef(gameobjectManager->getGameObject(val)));
                }

            }
        }

        return;
    }


};



}
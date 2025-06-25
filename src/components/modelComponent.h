#pragma once

#include "../fileDialog.h"

#include "../component.h"
#include "../resources/mesh.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class ModelComponent : public Component{
private:
    std::shared_ptr<Mesh> mesh;

public:
    ModelComponent(std::shared_ptr<ResourceManager> resMgr) : Component(resMgr), mesh(resMgr->getResource<Mesh>("./models/cubeuv.glb")){
        
    }

    ModelComponent(std::shared_ptr<Mesh> mesh): mesh(mesh){
        
    }
    
    std::vector<std::shared_ptr<VulkanBufferI>> getBuffers(){
        return mesh->getBuffers();
    }

    std::pair<uint32_t, uint32_t> getCount(){
        return {mesh->getVertexBuffer()->getVertexCount(), mesh->getIndexBuffer()->getIndexCount()};
    }

    void guiDisplayInspector(){
        if(ImGui::CollapsingHeader("Model")){
            
            ImGui::BeginGroup();

                ImGui::Text("Mesh: ");
                ImGui::SameLine();
                if(ImGui::SmallButton((mesh->getPath()).c_str())){

                    std::shared_ptr<Mesh> newMesh;

                    try{
                        std::string pth = std::filesystem::relative(FileDialog::fileDialog().getPath()).string();
                        if(!pth.empty())
                            newMesh = resourceManager->getResource<Mesh>(pth);
                    }catch(std::exception ex){
                        std::cout << "Can`t load mesh" << std::endl;
                    }

                    if(newMesh){
                        mesh = newMesh;
                    }
                }

            ImGui::EndGroup();

        }
    }


    json saveToJson(){
        json component;
        
        component["mesh"] = mesh->getPath();

        return component;
    }

    void loadFromJson(json component){

        mesh = resourceManager->getResource<Mesh>(component["mesh"].get<std::string>());

        return;
    }


};



}
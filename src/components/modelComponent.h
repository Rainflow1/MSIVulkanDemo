#pragma once

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
    ModelComponent(std::shared_ptr<Mesh> mesh): mesh(mesh){
        
    }
    
    std::vector<std::shared_ptr<VulkanBufferI>> getBuffers(){
        return mesh->getBuffers();
    }

    std::pair<uint32_t, uint32_t> getCount(){
        return {mesh->getVertexBuffer()->getVertexCount(), mesh->getIndexBuffer()->getIndexCount()};
    }

};



}
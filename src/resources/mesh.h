#pragma once

#include "../vulkan/vulkanCore.h"
#include "../resourceManager.h"

#include <iostream>
#include <vector>
#include <type_traits>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace MSIVulkanDemo{


class Mesh : public Resource{
private:
    std::map<const std::string, const uint32_t> supportedAttributes = {
        {"POSITION", 0},
        {"NORMAL", 1},
        {"TEXCOORD_0", 2}
    };

    std::vector<std::shared_ptr<VulkanBufferI>> buffers;
    std::shared_ptr<VulkanVertexBuffer> vertexBuffer;
    std::shared_ptr<VulkanIndexBuffer> indexBuffer;
    std::shared_ptr<VulkanMemoryManager> memoryManager;

    std::unique_ptr<VulkanVertexData> vertexData;

public:
    Mesh(std::string path){

        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        //bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);
        bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path); // for binary glTF(.glb)

        if (!warn.empty()) {
            std::cout << "glTF warn: " << warn << std::endl;
        }

        if (!err.empty()) {
            std::cout << "glTF error: " << err << std::endl;
        }

        if (!ret) {
            std::cout << "Failed to parse glTF file: " << path << std::endl;
        }

        auto& mesh = model.meshes[0]; //TODO support more meshes
        auto& primitive = mesh.primitives[0]; //TODO support more primitives

        //std::cout << mesh.primitives.size() << std::endl;
        //std::cout << model.meshes.size() << std::endl;

        std::map<uint32_t, std::pair<VkFormat, size_t>> attributes;
        std::map<uint32_t, std::pair<uint32_t, std::vector<float>>> attributesData;
        size_t vertexCount = 0;

        for(auto& [key, val] : primitive.attributes){ 
            auto& accessor = model.accessors[val];

            if(!supportedAttributes.count(key)){
                std::cout << "Attribute not supported: " << key << std::endl;
                continue;
            }
            
            auto& bufferView = model.bufferViews[accessor.bufferView];
            auto& buffer = model.buffers[bufferView.buffer];

            VkFormat format;

            switch (accessor.type){
                case TINYGLTF_TYPE_VEC3: // TODO support more types and component types
                    format = VK_FORMAT_R32G32B32_SFLOAT;
                    break;

                case TINYGLTF_TYPE_VEC2: 
                    format = VK_FORMAT_R32G32_SFLOAT;
                    break;
                
                default:
                    throw std::runtime_error("Not supported model");
                    break;
            }

            if(vertexCount == 0){
                vertexCount = accessor.count;
            }else if(vertexCount != accessor.count){
                throw std::runtime_error("Mesh attributes badly aligned");
            }

            attributesData.insert({supportedAttributes[key], {tinygltf::GetNumComponentsInType(accessor.type), std::vector<float>(bufferView.byteLength/sizeof(float))}});
            std::memcpy(attributesData[supportedAttributes[key]].second.data(), buffer.data.data() + bufferView.byteOffset, bufferView.byteLength);

            attributes.insert({supportedAttributes[key], {format, tinygltf::GetNumComponentsInType(accessor.type) * sizeof(float)}});
        }

        vertexData = std::unique_ptr<VulkanVertexData>(new VulkanVertexData(attributes));

        for(uint32_t i = 0; i < vertexCount; i++){
            std::vector<float> data;

            for(const auto& [key, val] : attributesData){
                for(uint32_t j = 0; j < val.first; j++){
                    data.push_back(val.second[val.first * i + j]);
                }
            }

            vertexData->append(data);
        }

        
        auto& indicesAccessor = model.accessors[primitive.indices];
        auto& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];
        auto& indicesBuffer = model.buffers[indicesBufferView.buffer];

        std::vector<uint32_t> indicesData;

        for(uint32_t i = 0; i < indicesBufferView.byteLength; i += tinygltf::GetComponentSizeInBytes(indicesAccessor.componentType)){
            uint32_t val = 0;
            for(uint32_t j = 0; j < tinygltf::GetComponentSizeInBytes(indicesAccessor.componentType); j++){
                reinterpret_cast<uint8_t*>(&val)[j] = indicesBuffer.data.data()[i + j + indicesBufferView.byteOffset];
            }
            indicesData.push_back(val);
        }

        vertexData->addIndices(indicesData);
    }

    ~Mesh(){}

    std::vector<std::shared_ptr<VulkanBufferI>> getBuffers(){
        return buffers;
    }

    std::shared_ptr<VulkanVertexBuffer> getVertexBuffer(){
        return vertexBuffer;
    }

    std::shared_ptr<VulkanIndexBuffer> getIndexBuffer(){
        return indexBuffer;
    }

private:

    void loadDependency(std::vector<std::any> dependencies){
        memoryManager = std::any_cast<std::shared_ptr<VulkanMemoryManager>>(dependencies[0]);

        vertexBuffer = std::shared_ptr<VulkanVertexBuffer>(new VulkanVertexBuffer(memoryManager, *vertexData));

        indexBuffer = std::shared_ptr<VulkanIndexBuffer>(new VulkanIndexBuffer(memoryManager, *vertexData));

        buffers.push_back(vertexBuffer);
        buffers.push_back(indexBuffer);
    }

};


}
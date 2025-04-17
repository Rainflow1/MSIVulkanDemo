#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <map>

namespace MSIVulkanDemo{

class VulkanVertexData{
private:

    std::vector<float> vertexData;
    std::map<std::string, VkVertexInputAttributeDescription> attributeDescriptions;
    std::vector<uint32_t> indexData;

    uint32_t attributeCount = 0;
    uint32_t attributeStride = 0;

    uint32_t vertexCount = 0;

public:
    VulkanVertexData(std::initializer_list<std::tuple<std::string, VkFormat, size_t>> attributes){

        for(auto tuple : attributes){
            VkFormat format = std::get<1>(tuple);

            VkVertexInputAttributeDescription attributeDescription;

            attributeDescription.binding = 0;
            attributeDescription.location = attributeCount;
            attributeDescription.format = format;
            attributeDescription.offset = attributeStride;

            attributeCount += 1;
            attributeStride += static_cast<uint32_t>(std::get<2>(tuple));
            attributeDescriptions.insert(std::pair(std::get<0>(tuple), attributeDescription));
        }

    }

    ~VulkanVertexData(){
        
    }

    VkVertexInputBindingDescription getBindingDescription(){
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = static_cast<uint32_t>(attributeStride);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(){
        std::vector<VkVertexInputAttributeDescription> vec;

        for(auto& pair: attributeDescriptions){
            vec.push_back(pair.second);
        }

        return vec;
    }

    float* data(){
        return vertexData.data();
    }

    size_t size(){
        return vertexData.size() * sizeof(float);
    }

    void append(std::initializer_list<float> val){
        
        if(sizeof(float) * val.size() != attributeStride){
            throw std::exception("Mismatch sizes of attribute and argument");
        }

        vertexData.insert(vertexData.end(), val.begin(), val.end());
        vertexCount += 1;
    }

    void append(std::initializer_list<std::initializer_list<float>> values){
        
        for(auto val : values){
            if(sizeof(float) * val.size() != attributeStride){
                throw std::exception("Mismatch sizes of attribute and argument");
            }
    
            vertexData.insert(vertexData.end(), val.begin(), val.end());
            vertexCount += 1;
        }
    }

    uint32_t getVertexCount(){
        return vertexCount;
    }

    void addIndices(std::initializer_list<uint32_t> indices){
        indexData.insert(indexData.end(), indices.begin(), indices.end());
    }

    bool hasIndices(){
        return indexData.size() > 0;
    }

    size_t getIndicesSize(){
        return indexData.size() * sizeof(uint32_t); 
    }

    uint32_t getIndicesCount(){
        return static_cast<uint32_t>(indexData.size()); 
    }

    uint32_t* getIndicesData(){
        return indexData.data();
    }

};

}
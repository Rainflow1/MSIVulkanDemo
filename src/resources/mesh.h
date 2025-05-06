#pragma once

#include "../vulkan/vulkanCore.h"
#include "../resourceManager.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class Mesh : public Resource{
private:
    std::vector<std::shared_ptr<VulkanBufferI>> buffers;
    std::shared_ptr<VulkanVertexBuffer> vertexBuffer;
    std::shared_ptr<VulkanIndexBuffer> indexBuffer;
    std::shared_ptr<VulkanMemoryManager> memoryManager;

public:
    Mesh(std::string path): Resource(path){

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

    void loadDependency(std::any dependency){
        memoryManager = std::any_cast<std::shared_ptr<VulkanMemoryManager>>(dependency);


        VulkanVertexData tak({{"pos", VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)}, {"col", VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3)}});

        tak.append({
            {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
            {0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f},
            {0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f},
            {-0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f},

            {-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f},
            {0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
            {0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
            {-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f}
        });

        tak.addIndices({
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4
        });

        vertexBuffer = std::shared_ptr<VulkanVertexBuffer>(new VulkanVertexBuffer(memoryManager, tak));
        indexBuffer = std::shared_ptr<VulkanIndexBuffer>(new VulkanIndexBuffer(memoryManager, tak));

        buffers.push_back(vertexBuffer);
        buffers.push_back(indexBuffer);
    }

};


}
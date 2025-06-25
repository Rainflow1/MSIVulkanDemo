#pragma once

#include <stb_image.h>

#include "../vulkan/vulkanCore.h"
#include "../resourceManager.h"

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{


class Texture : public Resource{
public:
    

private:
    std::shared_ptr<VulkanTexture> tex;
    std::shared_ptr<VulkanTextureView> texView;
    std::shared_ptr<VulkanTextureSampler> texSampler;
    std::shared_ptr<VulkanMemoryManager> memoryManager;

    std::unique_ptr<VulkanImageData> imageData;
    VulkanTexture::textureType type;

public:
    Texture(std::string path){
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        imageData = std::unique_ptr<VulkanImageData>(new VulkanImageData({texWidth, texHeight}, 4, 1));  // WARN only 4 channels supported

        std::vector<std::vector<uint8_t>> data;

        data.push_back(std::vector<uint8_t>(pixels, pixels + texWidth * texHeight * 4));  // WARN only 4 channels supported

        imageData->append(data);

        stbi_image_free(pixels);
    }

    Texture(std::vector<std::string> paths, VulkanTexture::textureType type = VulkanTexture::Cubemap): type(type){

        int texWidth, texHeight, texChannels;
        stbi_image_free(stbi_load(paths[0].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha));

        imageData = std::unique_ptr<VulkanImageData>(new VulkanImageData({texWidth, texHeight}, 4, 6));  // WARN only 4 channels supported

        std::vector<std::vector<uint8_t>> data;

        for(std::string path : paths){
            stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            data.push_back(std::vector<uint8_t>(pixels, pixels + texWidth * texHeight * 4));  // WARN only 4 channels supported

            //std::cout << path << ": " << texWidth << ", " << texHeight << std::endl;

            stbi_image_free(pixels);
        }

        imageData->append(data);
    }

    ~Texture(){}

    VulkanTextureView& getTextureView(){
        return *texView;
    }

    VulkanTextureSampler& getTextureSampler(){
        return *texSampler;
    }

private:

    void loadDependency(std::vector<std::any> dependencies){
        memoryManager = std::any_cast<std::shared_ptr<VulkanMemoryManager>>(dependencies[0]);

        tex = std::shared_ptr<VulkanTexture>(new VulkanTexture(memoryManager, *imageData, type));
        texView = std::shared_ptr<VulkanTextureView>(new VulkanTextureView(tex));
        texSampler = memoryManager->getDevice()->createTextureSampler();

        return;
    }

};


}
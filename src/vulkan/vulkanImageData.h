#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <map>

namespace MSIVulkanDemo{

class VulkanImageData{
private:

    std::vector<uint8_t> imageData;
    std::pair<uint32_t, uint32_t> resolution;
    uint32_t channels, layers;

public:
    VulkanImageData(std::pair<uint32_t, uint32_t> resolution, uint32_t channels, uint32_t layers = 1):resolution(resolution), channels(channels), layers(layers){

    }

    ~VulkanImageData(){
        
    }

    void append(std::vector<std::vector<uint8_t>> data){

        if(data.size() != layers){
            throw std::exception("Mismatch layers size of data");
        }

        for(auto layerData : data){
            if(layerData.size() != resolution.first * resolution.second * channels){
                throw std::exception("Mismatch size of layer");
            }

            imageData.insert(imageData.end(), layerData.begin(), layerData.end());
        }

    }

    std::pair<uint32_t, uint32_t> getResolution(){
        return resolution;
    }

    uint32_t getChannelsNum(){
        return channels;
    }

    uint32_t getLayersNum(){
        return layers;
    }

    uint8_t* data(){
        return imageData.data();
    }

    size_t size(){
        return resolution.first * resolution.second * channels * layers;
    }

};

}
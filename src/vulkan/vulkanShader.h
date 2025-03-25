#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>

#include "interface/vulkanDeviceI.h"

#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>

namespace MSIVulkanDemo{

enum ShaderType{
    Vertex = shaderc_vertex_shader,
    Fragment = shaderc_fragment_shader
};

class VulkanShader{
private:
    std::shared_ptr<VulkanDeviceI> device;

    inline static shaderc::Compiler* compiler = nullptr;

    VkShaderModule shaderModule = nullptr;

public:
    VulkanShader(std::shared_ptr<VulkanDeviceI> device, const std::string& filename, ShaderType shaderType): device(device){

        if(compiler == nullptr){
            compiler = new shaderc::Compiler(); // Lazy init of dangling global compiler
            std::cout << "Shader compiler initialized" << std::endl;
        }

        std::string code = readShaderFile(filename);
        std::string preprocesedCode = preprocessGLSL(filename, shaderType, code);
        std::cout << preprocesedCode << std::endl;
        std::vector<uint32_t> compiledCode = compileGLSL(filename, shaderType, preprocesedCode);

        //printCompiledCode(compiledCode);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = compiledCode.size() * sizeof(uint32_t);
        createInfo.pCode = compiledCode.data();

        if (vkCreateShaderModule(device->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        // TODO modół shader dok plus modól pipeline który może być konfigurowalny za pomoca shaderów plus modół resorces z menagerem, złączanie shaderów, include (hot reload shaderów do projektu)
    }

    ~VulkanShader(){
        if(shaderModule){
            vkDestroyShaderModule(device->getDevice(), shaderModule, nullptr);
        }
    }

    operator VkShaderModule() const{
        return shaderModule;
    }

private:

    static std::string readShaderFile(const std::string& filename){
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::string buffer(fileSize, '\0');

        file.seekg(0);
        file.read(&buffer[0], fileSize);

        file.close();

        return buffer;
    }

    std::string preprocessGLSL(const std::string& source_name, ShaderType kind, const std::string& source) {
        shaderc::CompileOptions options;

        switch (kind){
        case Vertex:
            options.AddMacroDefinition("VERTEX", "1");
            break;

        case Fragment:
            options.AddMacroDefinition("FRAGMENT", "1");
            break;

        default:
            break;
        }

        shaderc::PreprocessedSourceCompilationResult result = compiler->PreprocessGlsl(source, static_cast<shaderc_shader_kind>(kind), source_name.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return "";
        }

        std::string stringResult = {result.cbegin(), result.cend()};
        stringResult.erase(std::unique(stringResult.begin(), stringResult.end(), [] (char a, char b) {return a == '\n' && b == '\n';}), stringResult.end());

        return stringResult;
    }


    std::vector<uint32_t> compileGLSL(const std::string& source_name, ShaderType kind, const std::string& source, bool optimize = false){
        shaderc::CompileOptions options;

        //options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::SpvCompilationResult module = compiler->CompileGlslToSpv(source, static_cast<shaderc_shader_kind>(kind), source_name.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << module.GetErrorMessage();
            return std::vector<uint32_t>();
        }

        return {module.cbegin(), module.cend()};
    }

    std::string compileAssembly(const std::string& source_name, shaderc_shader_kind kind, const std::string& source, bool optimize = false){
        shaderc::CompileOptions options;

        //options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::AssemblyCompilationResult result = compiler->CompileGlslToSpvAssembly(source, kind, source_name.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return "";
        }

        return {result.cbegin(), result.cend()};
    }

    void printCompiledCode(std::vector<uint32_t>& compiledCode){
        int nl = 0;
        for(uint32_t i : compiledCode){
            unsigned char* word = static_cast<unsigned char*>(static_cast<void*>(&i));

            for(int i = 0; i < 4; i++){
                unsigned int byte = word[i];
                std::cout << std::hex << std::setfill('0') << std::setw(2) << byte << " ";
            }

            if(++nl%4 == 0){
                std::cout << std::endl;
            }
        }
        std::cout << std::dec << std::endl;
    }
};

}
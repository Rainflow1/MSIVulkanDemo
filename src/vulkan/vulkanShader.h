#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <shaderc/shaderc.hpp>
#include "spirv_reflect.h"

#include "interface/vulkanDeviceI.h"
#include "vulkanVertexData.h"
#include "vulkanUniform.h"

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
    SpvReflectShaderModule module;
    std::vector<uint32_t> compiledCode;
    ShaderType type;

public:
    VulkanShader(std::shared_ptr<VulkanDeviceI> device, const std::string& filename, ShaderType shaderType): device(device), type(shaderType){

        if(compiler == nullptr){
            compiler = new shaderc::Compiler(); // Lazy init of dangling global compiler
            std::cout << "Shader compiler initialized" << std::endl;
        }

        std::string code = readShaderFile(filename);
        std::string preprocesedCode = preprocessGLSL(filename, shaderType, code);
        //std::cout << preprocesedCode << std::endl;
        compiledCode = compileGLSL(filename, shaderType, preprocesedCode);

        if(spvReflectCreateShaderModule(compiledCode.size() * sizeof(uint32_t), compiledCode.data(), &module) != SPV_REFLECT_RESULT_SUCCESS){
            throw std::runtime_error("Cannot create reflect module");
        }

        //printCompiledCode(compiledCode);
        //reflectTest(compiledCode);

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
        spvReflectDestroyShaderModule(&module);
    }

    operator VkShaderModule() const{
        return shaderModule;
    }

    ShaderType getType(){
        return type;
    }

    VulkanVertexData getVertexData(){ // TODO support location
        
        uint32_t num = 0;
        if(spvReflectEnumerateInputVariables(&module, &num, NULL) != SPV_REFLECT_RESULT_SUCCESS){
            throw std::runtime_error("Cannot fetch input var number");
        }

        SpvReflectInterfaceVariable** attribs = (SpvReflectInterfaceVariable**)malloc(num * sizeof(SpvReflectInterfaceVariable*));

        if(spvReflectEnumerateInputVariables(&module, &num, attribs) != SPV_REFLECT_RESULT_SUCCESS){
            throw std::runtime_error("Cannot fetch input vars");
        }

        std::map<uint32_t, std::pair<VkFormat, size_t>> attributes;

        //std::vector<std::tuple<std::string, VkFormat, size_t>> attributes;

        for(uint32_t i = 0; i < num; i++){
            //attributes.push_back({std::string(input_vars[i]->name), static_cast<VkFormat>(input_vars[i]->format), input_vars[i]->numeric.scalar.width/8 * input_vars[i]->numeric.vector.component_count /* TODO calculate size also for mats and arrays */});
            attributes.insert({attribs[i]->location, {static_cast<VkFormat>(attribs[i]->format), attribs[i]->numeric.scalar.width/8 * attribs[i]->numeric.vector.component_count}});
        }

        return VulkanVertexData(attributes);
    }

    VulkanUniformData* getUniformData(){

        uint32_t varCount = 0;
        if(spvReflectEnumerateDescriptorSets(&module, &varCount, NULL) != SPV_REFLECT_RESULT_SUCCESS){
            throw std::runtime_error("Cannot fetch input var number");
        }

        SpvReflectDescriptorSet** inputVars = (SpvReflectDescriptorSet**)malloc(varCount * sizeof(SpvReflectDescriptorSet*));

        if(spvReflectEnumerateDescriptorSets(&module, &varCount, inputVars) != SPV_REFLECT_RESULT_SUCCESS){
            throw std::runtime_error("Cannot fetch input vars");
        }

        std::vector<VulkanUniformData::bindingBlock> blocks; 

        for(uint32_t i = 0; i < varCount; i++){

            for(uint32_t j = 0; j < inputVars[i]->binding_count; j++){
                auto binding = inputVars[i]->bindings[j];

                VulkanUniformData::bindingBlock block;
                block.set = inputVars[i]->set;
                block.binding = binding->binding;
                block.type = static_cast<VkDescriptorType>(binding->descriptor_type);

                if(block.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER){
                    block.attribs.push_back({
                        .binding = block.binding, 
                        .name = binding->name, 
                        .size = 0, 
                        .componentCount = static_cast<uint32_t>(binding->image.dim == SpvDimCube ? 6 : 0) // TODO vector support
                    });
                }

                for(uint32_t k = 0; k < binding->block.member_count; k++){
                    auto member = binding->block.members[k];

                    VulkanUniformData::attribute attrib; 
                    attrib.name = member.name;
                    attrib.size = member.padded_size;
                    attrib.binding = block.binding;

                    if(member.numeric.matrix.column_count > 0){
                        attrib.componentCount = member.numeric.matrix.column_count * member.numeric.matrix.row_count;
                    }else if(member.numeric.vector.component_count > 0){
                        attrib.componentCount = member.numeric.vector.component_count;
                    }else{
                        attrib.componentCount = 1;
                    }
                    
                    block.attribs.push_back(attrib);
                }

                blocks.push_back(block);
            }

        }

        return new VulkanUniformData(blocks);
    }

private:

    static std::string readShaderFile(const std::string& filename){
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file: " + filename);
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

    void reflectTest(std::vector<uint32_t>& compiledCode){
        SpvReflectShaderModule module;
        SpvReflectResult result = spvReflectCreateShaderModule(compiledCode.size() * sizeof(uint32_t), compiledCode.data(), &module);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
      
        // Enumerate and extract shader's input variables
        uint32_t var_count = 0;
        result = spvReflectEnumerateInputVariables(&module, &var_count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        SpvReflectInterfaceVariable** input_vars = (SpvReflectInterfaceVariable**)malloc(var_count * sizeof(SpvReflectInterfaceVariable*));
        result = spvReflectEnumerateInputVariables(&module, &var_count, input_vars);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        for(int i = 0; i < var_count; i++){
            std::cout << input_vars[i]->name << ": " << input_vars[i]->format << " - " << input_vars[i]->numeric.scalar.width * input_vars[i]->numeric.vector.component_count << std::endl;
        }


        var_count = 0;
        result = spvReflectEnumerateDescriptorSets(&module, &var_count, NULL);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        SpvReflectDescriptorSet** input_vars2 = (SpvReflectDescriptorSet**)malloc(var_count * sizeof(SpvReflectDescriptorSet*));
        result = spvReflectEnumerateDescriptorSets(&module, &var_count, input_vars2);
        assert(result == SPV_REFLECT_RESULT_SUCCESS);

        for(int i = 0; i < var_count; i++){
            std::cout << "Set " << input_vars2[i]->set << ": " << std::endl;
            for(int j = 0; j < input_vars2[i]->binding_count; j++){
                auto binding = input_vars2[i]->bindings[j];
                std::cout << "Binding: " << binding->binding << ": " << binding->block.member_count << std::endl;
                for(int k = 0; k < binding->block.member_count; k++){
                    auto block = binding->block.members[k];
                    std::cout << block.name << ": " << block.size << std::endl;
                }
            }
        }

      
        // Output variables, descriptor bindings, descriptor sets, and push constants
        // can be enumerated and extracted using a similar mechanism.
      
        // Destroy the reflection data when no longer required.
        spvReflectDestroyShaderModule(&module);
    }

    

};



}
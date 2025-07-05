#pragma once

#include <GLFW/glfw3.h>

#define NFD_OVERRIDE_RECENT_WITH_DEFAULT
#include <nfd_glfw3.h>
#include <nfd.hpp>

#include <iostream>
#include <vector>
#include <type_traits>

namespace MSIVulkanDemo{

class FileDialog{

private:
    inline static FileDialog* singleton = nullptr;
    nfdwindowhandle_t NFDWINDOW;

public:
    FileDialog(GLFWwindow* window){
        NFD_Init();
        NFD_GetNativeWindowFromGLFWWindow(window, &NFDWINDOW);
    }

    ~FileDialog(){
        NFD_Quit();
    }

    void clear(){
        delete singleton;
    }

    static FileDialog& fileDialog(GLFWwindow* window){
        singleton = new FileDialog(window);
        return *singleton;
    }

    static FileDialog& fileDialog(){
        if(!singleton){
            throw std::runtime_error("File dialog not initialized");
        }
        return *singleton;
    }

    std::string savePath(std::string defaultName, std::string defaultPath = "./"){
        std::string path;
        nfdu8char_t *outPath;
        nfdsavedialogu8args_t args = {0};
        args.parentWindow = NFDWINDOW;
        args.defaultName = defaultName.c_str();
        std::string absDefPath = std::filesystem::absolute(defaultPath).string();
        args.defaultPath = absDefPath.c_str();

        nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);

        if (result == NFD_OKAY){
            path = std::string(outPath);
            NFD_FreePathU8(outPath);
        }else if (result == NFD_CANCEL){
            path = "";
        }else{
            std::cout << "FileDialog error" << std::endl;
            path = "";
        }

        return std::filesystem::relative(path).string();
    }

    std::string getPath(std::string defaultPath = "./"){
        std::string path;
        nfdu8char_t *outPath;
        nfdopendialogu8args_t args = {0};
        args.parentWindow = NFDWINDOW;
        std::string absDefPath = std::filesystem::absolute(defaultPath).string();
        args.defaultPath = absDefPath.c_str();

        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);

        if (result == NFD_OKAY){
            path = std::string(outPath);
            NFD_FreePathU8(outPath);
        }else if (result == NFD_CANCEL){
            path = "";
        }else{
            std::cout << "FileDialog error" << std::endl;
            path = "";
        }

        return std::filesystem::relative(path).string();
    }

    std::string getPaths(std::string defaultPath = "./"){
        std::string path = "";
        const nfdpathset_t *outPaths;
        nfdopendialogu8args_t args = {0};
        args.parentWindow = NFDWINDOW;
        std::string absDefPath = std::filesystem::absolute(defaultPath).string();
        args.defaultPath = absDefPath.c_str();

        nfdresult_t result = NFD_OpenDialogMultipleU8_With(&outPaths, &args);

        if (result == NFD_OKAY){

            nfdpathsetsize_t numPaths;
            NFD_PathSet_GetCount(outPaths, &numPaths);

            for(uint32_t i = 0; i < numPaths; i++){
                nfdchar_t* outPath;
                NFD_PathSet_GetPath(outPaths, i, &outPath);

                path += std::filesystem::relative(std::string(outPath)).string() + ";";

                NFD_PathSet_FreePath(outPath);
            }
            path.pop_back();
            NFD_PathSet_Free(outPaths);
        }else if (result == NFD_CANCEL){
            path = "";
        }else{
            std::cout << "FileDialog error" << std::endl;
            path = "";
        }

        return path;
    }

};

}

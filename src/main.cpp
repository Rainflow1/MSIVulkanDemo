#include "app.h"

int main(){
    
    MSIVulkanDemo::App app;

    try{
        app.run();
    }catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }

    return 0;
}
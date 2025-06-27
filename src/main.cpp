#include "app.h"
#include <stdlib.h>

int main(){

    try{
        MSIVulkanDemo::App app;
        app.run();
    }catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }

#ifdef RELEASE
    system("pause");
#endif
    return 0;
}
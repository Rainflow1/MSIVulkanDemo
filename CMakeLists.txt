cmake_minimum_required(VERSION 3.26)

project(MSIVulkanDemo)

set(CMAKE_CXX_STANDARD 20)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(EnTT REQUIRED)
find_package(unofficial-spirv-reflect REQUIRED)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(Stb REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(nfd REQUIRED)
find_package(pybind11 REQUIRED)

add_executable(MSIVulkanDemo src/main.cpp)

target_compile_options(MSIVulkanDemo PUBLIC "/bigobj")

target_link_libraries(MSIVulkanDemo glfw)
target_link_libraries(MSIVulkanDemo glm::glm)
target_link_libraries(MSIVulkanDemo Vulkan::Vulkan)
target_link_libraries(MSIVulkanDemo imgui::imgui)
target_link_libraries(MSIVulkanDemo unofficial::shaderc::shaderc)
target_link_libraries(MSIVulkanDemo GPUOpen::VulkanMemoryAllocator)
target_link_libraries(MSIVulkanDemo EnTT::EnTT)
target_link_libraries(MSIVulkanDemo unofficial::spirv-reflect)
target_include_directories(MSIVulkanDemo PRIVATE ${TINYGLTF_INCLUDE_DIRS})
target_include_directories(MSIVulkanDemo PRIVATE ${Stb_INCLUDE_DIR})
target_link_libraries(MSIVulkanDemo nlohmann_json::nlohmann_json)
target_link_libraries(MSIVulkanDemo nfd::nfd)
target_link_libraries(MSIVulkanDemo pybind11::embed)

add_custom_target(copy_shaders
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
        "$<TARGET_FILE_DIR:MSIVulkanDemo>/shaders"
    COMMENT "Copying shaders"
)
add_dependencies(MSIVulkanDemo copy_shaders)

add_custom_target(copy_models
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/models/"
        "$<TARGET_FILE_DIR:MSIVulkanDemo>/models"
    COMMENT "Copying models"
)
add_dependencies(MSIVulkanDemo copy_models)

add_custom_target(copy_textures
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/textures/"
        "$<TARGET_FILE_DIR:MSIVulkanDemo>/textures"
    COMMENT "Copying textures"
)
add_dependencies(MSIVulkanDemo copy_textures)

add_custom_target(copy_scenes
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/scenes/"
        "$<TARGET_FILE_DIR:MSIVulkanDemo>/scenes"
    COMMENT "Copying scenes"
)
add_dependencies(MSIVulkanDemo copy_scenes)

add_custom_target(copy_scripts
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/"
        "$<TARGET_FILE_DIR:MSIVulkanDemo>/scripts"
    COMMENT "Copying scripts"
)
add_dependencies(MSIVulkanDemo copy_scripts)

add_custom_target(copy_imgui_config
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini"
        "$<TARGET_FILE_DIR:MSIVulkanDemo>/imgui.ini"
    COMMENT "Copying imgui config"
)
add_dependencies(MSIVulkanDemo copy_imgui_config)
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

add_executable(main src/main.cpp)

target_compile_options(main PUBLIC "/bigobj")

target_link_libraries(main glfw)
target_link_libraries(main glm::glm)
target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main imgui::imgui)
target_link_libraries(main unofficial::shaderc::shaderc)
target_link_libraries(main GPUOpen::VulkanMemoryAllocator)
target_link_libraries(main EnTT::EnTT)
target_link_libraries(main unofficial::spirv-reflect)
target_include_directories(main PRIVATE ${TINYGLTF_INCLUDE_DIRS})
target_include_directories(main PRIVATE ${Stb_INCLUDE_DIR})


add_custom_target(copy_shaders
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
        "$<TARGET_FILE_DIR:main>/shaders"
    COMMENT "Copying shaders"
)
add_dependencies(main copy_shaders)

add_custom_target(copy_models
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/models/"
        "$<TARGET_FILE_DIR:main>/models"
    COMMENT "Copying models"
)
add_dependencies(main copy_models)

add_custom_target(copy_textures
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/textures/"
        "$<TARGET_FILE_DIR:main>/textures"
    COMMENT "Copying textures"
)
add_dependencies(main copy_textures)
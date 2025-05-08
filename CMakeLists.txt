cmake_minimum_required(VERSION 3.26)

project(MSIVulkanDemo)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(EnTT REQUIRED)
find_package(unofficial-spirv-reflect REQUIRED)

add_executable(main src/main.cpp)

target_link_libraries(main glfw)
target_link_libraries(main glm::glm)
target_link_libraries(main Vulkan::Vulkan)
target_link_libraries(main imgui::imgui)
target_link_libraries(main unofficial::shaderc::shaderc)
target_link_libraries(main GPUOpen::VulkanMemoryAllocator)
target_link_libraries(main EnTT::EnTT)
target_link_libraries(main unofficial::spirv-reflect)


add_custom_target(copy_shaders
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/"
        "$<TARGET_FILE_DIR:main>/shaders"
    COMMENT "Copying shaders"
)
add_dependencies(main copy_shaders)

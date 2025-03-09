cmake_minimum_required(VERSION 3.10)

project(MSIVulkanDemo)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(imgui REQUIRED)

include_directories(C:/VulkanSDK/1.4.304.1/Include)
link_directories(C:/VulkanSDK/1.4.304.1/Lib)

add_executable(main src/main.cpp)

target_link_libraries(main glfw)
target_link_libraries(main glm::glm)
target_link_libraries(main vulkan-1)
target_link_libraries(main imgui::imgui)
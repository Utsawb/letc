cmake_minimum_required(VERSION 3.24)
project(letc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(EXTERNAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external)

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS})

find_package(Vulkan REQUIRED)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${VULKAN_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Vulkan::Vulkan)

add_subdirectory(${EXTERNAL_DIR}/OpenXR-SDK ${CMAKE_CURRENT_BINARY_DIR}/openxr-build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE openxr_loader)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${EXTERNAL_DIR}/OpenXR-SDK/include)

add_subdirectory(${EXTERNAL_DIR}/glm ${CMAKE_CURRENT_BINARY_DIR}/glm-build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glm::glm)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${EXTERNAL_DIR}/glm)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(${EXTERNAL_DIR}/glfw ${CMAKE_CURRENT_BINARY_DIR}/glfw-build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${EXTERNAL_DIR}/vkfw/include/)

add_subdirectory(${EXTERNAL_DIR}/VulkanMemoryAllocator ${CMAKE_CURRENT_BINARY_DIR}/vma-build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Build Assimp tests" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Build Assimp tools" FORCE)
add_subdirectory(${EXTERNAL_DIR}/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp-build)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE assimp::assimp)

target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hh")

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 26)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
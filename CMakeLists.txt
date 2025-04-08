cmake_minimum_required(VERSION 3.24)
project(letc)
include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")
file(GLOB_RECURSE GLSL "resources/*.glsl")
file(GLOB_RECURSE GLTF "resources/*.gltf")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${OPENXRHPP} ${GLSL} ${GLTF})

target_include_directories(${CMAKE_PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

find_package(Vulkan REQUIRED)
include_directories(${VULKAN_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Vulkan::Vulkan)

FetchContent_Declare(
  openxr
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK-Source.git
  GIT_TAG        release-1.0.26
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(openxr)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE openxr_loader)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${openxr_SOURCE_DIR}/include)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${glm_SOURCE_DIR})

FetchContent_Declare(
  glfw3
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw3)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw)

FetchContent_Declare(
  vkfw
  GIT_REPOSITORY https://github.com/Cvelth/vkfw.git
  GIT_TAG        main
)
FetchContent_MakeAvailable(vkfw)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${vkfw_SOURCE_DIR}/include)

FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v3.2.1
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Build Assimp tests" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Build Assimp tools" FORCE)
FetchContent_MakeAvailable(assimp)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE assimp::assimp)

FetchContent_Declare(
  Eigen3
  GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
  GIT_TAG        3.4.0
)
FetchContent_MakeAvailable(Eigen3)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Eigen3::Eigen)

target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hh")

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 26)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
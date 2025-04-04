cmake_minimum_required(VERSION 3.24)
project(letc)
include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SOURCES "src/*.cc")
file(GLOB_RECURSE HEADERS "src/*.hh")
file(GLOB_RECURSE GLSL "resources/*.glsl")
file(GLOB_RECURSE GLTF "resources/*.gltf")

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES} ${HEADERS} ${GLSL} ${GLTF})

find_package(Vulkan REQUIRED)
include_directories(${VULKAN_INCLUDE_DIRS})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Vulkan::Vulkan)

# --- OpenXR ---
FetchContent_Declare(
  openxr
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK-Source.git
  GIT_TAG        release-1.1.46 # Using a specific release tag is good practice
  OVERRIDE_FIND_PACKAGE
)
# This fetches the source and runs its CMakeLists.txt, defining the openxr_loader target
FetchContent_MakeAvailable(openxr)
# Link against the target created by OpenXR's CMakeLists.txt
# This also handles include directories for OpenXR correctly.
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE openxr_loader)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${openxr_SOURCE_DIR}/include)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glm)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${glm_SOURCE_DIR})

FetchContent_Declare(
  glfw3
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(glfw3)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw)
# target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${glfw3_SOURCE_DIR}/include)

FetchContent_Declare(
  vkfw
  GIT_REPOSITORY https://github.com/Cvelth/vkfw.git
  GIT_TAG        main
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(vkfw)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${vkfw_SOURCE_DIR}/include)

FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG        v3.2.1
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)
# target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${VulkanMemoryAllocator_SOURCE_DIR}/include)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
    OVERRIDE_FIND_PACKAGE
)
# Prevent assimp from building tests/tools if not needed
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Build Assimp tests" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "Build Assimp tools" FORCE)
FetchContent_MakeAvailable(assimp)
# Linking the imported target 'assimp::assimp' handles includes and libs
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE assimp::assimp)

FetchContent_Declare(
  spirv-cross
  GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
  GIT_TAG        main
)
# Prevent SPIRV-Cross from building executables/tests if only the library is needed
set(SPIRV_CROSS_CLI OFF CACHE BOOL "Build SPIRV-Cross CLI" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Build SPIRV-Cross tests" FORCE)
# You might need to set other options depending on required backends, e.g.:
# set(SPIRV_CROSS_ENABLE_GLSL=ON CACHE BOOL "" FORCE)
# set(SPIRV_CROSS_ENABLE_HLSL=ON CACHE BOOL "" FORCE)
# set(SPIRV_CROSS_ENABLE_MSL=ON CACHE BOOL "" FORCE)
# set(SPIRV_CROSS_ENABLE_CPP=ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spirv-cross)

# Link against the core library target (adjust if needed)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE spirv-cross-core)
# Include the source directory (adjust if headers are in a subdir like 'include')
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${spirv-cross_SOURCE_DIR})
# --- End SPIRV-Cross ---


# Add precompiled headers only to your main executable target
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.hh")

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 26)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

cmake_minimum_required(VERSION 3.30)
project(letc-dev LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(FetchContent)

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_VERSION}")

find_package(glm QUIET)
if(NOT glm_FOUND)
    message(STATUS "glm not found system-wide. Fetching from source...")
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        1.0.1
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(glm)
    find_package(glm REQUIRED)
else()
    message(STATUS "Found glm: ${GLM_VERSION_STRING}")
endif()

find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "glfw3 not found system-wide. Fetching from source...")
    FetchContent_Declare(
        glfw3
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        OVERRIDE_FIND_PACKAGE
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw3)
    find_package(glfw3 REQUIRED)
else()
    message(STATUS "Found glfw3: ${glfw3_VERSION_STRING}")
endif()


FetchContent_Declare(
    vkfw
    GIT_REPOSITORY https://github.com/Cvelth/vkfw.git
    GIT_TAG        main
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(vkfw)

set(TINYGLTF_HEADER_ONLY ON CACHE INTERNAL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE INTERNAL "" FORCE)
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf
    GIT_TAG v2.9.5
    OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(tinygltf)

find_package(VulkanMemoryAllocator QUIET)
if(NOT VulkanMemoryAllocator_FOUND)
    message(STATUS "VulkanMemoryAllocator not found system-wide. Fetching from source...")
    FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG        v3.2.1
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(VulkanMemoryAllocator)
    find_package(VulkanMemoryAllocator REQUIRED)
else()
    message(STATUS "Found VulkanMemoryAllocator: ${VulkanMemoryAllocator_VERSION}")
endif()

find_package(Slang QUIET)
if (NOT Slang_FOUND)
    message(STATUS "Slang not found system-wide, Fetching from source...")
    FetchContent_Declare(
        Slang
        GIT_REPOSITORY https://github.com/shader-slang/slang.git
        GIT_TAG v2025.7
        OVERRIDE_FIND_PACKAGE
    )
    FetchContent_MakeAvailable(Slang)
    find_package(Slang REQUIRED)
else()
    find_package(Slang)
endif()

add_subdirectory(letc)

file(GLOB TESTBED_DIRS LIST_DIRECTORIES true "tests/*")
foreach(TESTBED_DIR ${TESTBED_DIRS})
    if(IS_DIRECTORY ${TESTBED_DIR} AND EXISTS "${TESTBED_DIR}/CMakeLists.txt")
        message(STATUS "Adding test: ${TESTBED_DIR}")
        add_subdirectory(${TESTBED_DIR})
    endif()
endforeach()

message(STATUS "Top-level CMake configuration finished.")

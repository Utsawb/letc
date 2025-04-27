cmake_minimum_required(VERSION 3.30)
project(letc-dev LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 26)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(FetchContent)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(glfw3 REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spirv_cross_reflect REQUIRED)

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

add_subdirectory(letc)

file(GLOB TESTBED_DIRS LIST_DIRECTORIES true "tests/*")
foreach(TESTBED_DIR ${TESTBED_DIRS})
    if(IS_DIRECTORY ${TESTBED_DIR} AND EXISTS "${TESTBED_DIR}/CMakeLists.txt")
        message(STATUS "Adding test: ${TESTBED_DIR}")
        add_subdirectory(${TESTBED_DIR})
    endif()
endforeach()

message(STATUS "Top-level CMake configuration finished.")

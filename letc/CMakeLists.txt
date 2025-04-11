file(GLOB_RECURSE ENGINE_SOURCES "src/*.cc")
add_library(letc STATIC ${ENGINE_SOURCES})

set_target_properties(letc PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})
set_target_properties(letc PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(letc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# --- Link Engine Dependencies ---
# Link dependencies required INTERNALLY by the engine library.
# Use PUBLIC if the dependency's headers are included in YOUR PUBLIC headers (letc/include).
# Use PRIVATE if the dependency is only used within your source files (letc/src).
# Use INTERFACE if it's header-only or only affects the interface.

target_link_libraries(letc
    PUBLIC
        glm::glm

    PRIVATE
        Vulkan::Vulkan
        OpenXR::openxr_loader
        GPUOpen::VulkanMemoryAllocator
        assimp::assimp
        Eigen3::Eigen
        Taskflow::Taskflow
)

target_include_directories(letc
    PUBLIC
        ${GLM_INCLUDE_DIRS}
        ${vkfw_SOURCE_DIR}/include
)

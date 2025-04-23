file(GLOB_RECURSE ENGINE_SOURCES "**.cc")
add_library(letc STATIC ${ENGINE_SOURCES})

set_target_properties(letc PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})
set_target_properties(letc PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(letc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/
)

target_precompile_headers(letc
    PRIVATE
        letc/pch.hh
)

target_link_libraries(letc
    PUBLIC
        glm::glm
        Vulkan::Vulkan
        GPUOpen::VulkanMemoryAllocator
        tinygltf
)

target_include_directories(letc
    PUBLIC
        ${GLM_INCLUDE_DIRS}
        ${vkfw_SOURCE_DIR}/include
        ${tinyglft_SOURCE_DIR}/
)

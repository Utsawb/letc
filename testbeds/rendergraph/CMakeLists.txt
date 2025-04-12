set(TESTBED_NAME "rendergraph")

file(GLOB_RECURSE TESTBED_SOURCES "src/*.cc")
file(GLOB_RECURSE TESTBED_HEADERS "src/*.hh")

add_executable(${TESTBED_NAME} ${TESTBED_SOURCES} ${TESTBED_HEADERS})

set_target_properties(${TESTBED_NAME} PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})
set_target_properties(${TESTBED_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET ${TESTBED_NAME} PROPERTY FOLDER "Testbeds")

target_link_libraries(${TESTBED_NAME}
    PRIVATE
        letc
        glfw
        Vulkan::Vulkan
)

target_include_directories(${TESTBED_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/generated
)

set(TESTBED_NAME "basic_setup")

file(GLOB_RECURSE TESTBED_SOURCES "**.cc")
file(GLOB_RECURSE TESTBED_HEADERS "**.hh")

add_executable(${TESTBED_NAME} ${TESTBED_SOURCES} ${TESTBED_HEADERS})

set_target_properties(${TESTBED_NAME} PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD}) 
set_target_properties(${TESTBED_NAME} PROPERTIES LINKER_LANGUAGE CXX)
set_property(TARGET ${TESTBED_NAME} PROPERTY FOLDER "Tests")

target_link_libraries(${TESTBED_NAME}
    letc
    glfw
    Vulkan::Vulkan
)

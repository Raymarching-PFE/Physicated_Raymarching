project(Vulkan_Renderer)

set(MY_BIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Build/${CMAKE_CXX_COMPILER_ID}_$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MY_BIN_OUTPUT_DIR}")

option(COMPUTE "Use Compute pipeline" ON)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

file(GLOB_RECURSE MY_SOURCES "source/*.cpp")

add_executable(${PROJECT_NAME}
        ${MY_SOURCES}
)

if (COMPUTE)
    MESSAGE(STATUS "COMPUTE: ON")
    target_compile_definitions(${PROJECT_NAME} PRIVATE COMPUTE)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${tracy_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
        PRIVATE glfw
        PRIVATE glm
        PRIVATE stb_image
        PRIVATE tinyobjloader
        PRIVATE happly
        PRIVATE Vulkan::Vulkan
        PRIVATE Vulkan::shaderc_combined
        PRIVATE imgui
        PRIVATE Tracy::TracyClient
)

target_compile_features(${PROJECT_NAME}
        PRIVATE c_std_11
        PRIVATE cxx_std_20
)

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/point_clouds
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/point_clouds
)
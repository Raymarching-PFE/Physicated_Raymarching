project(Vulkan_Renderer)

set(MY_BIN_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/Build/${CMAKE_CXX_COMPILER_ID}_$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MY_BIN_OUTPUT_DIR}")

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE MY_SOURCES "source/*.cpp")

add_executable(${PROJECT_NAME}
        ${MY_SOURCES}
)


target_link_libraries(${PROJECT_NAME}
        PRIVATE glfw
        PRIVATE glm
        PRIVATE stb_image
        PRIVATE tinyobjloader
        PRIVATE Vulkan::Vulkan
        PRIVATE imgui
)

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/models

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/textures
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures
)
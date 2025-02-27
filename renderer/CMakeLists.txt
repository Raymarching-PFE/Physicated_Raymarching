project(Vulkan_Renderer
        LANGUAGES CXX C
        VERSION 1.0
        DESCRIPTION "Vulkan_Renderer"
)


find_package(Vulkan REQUIRED)


add_executable(${PROJECT_NAME}
        main.cpp
        vulkan_renderer.cpp
)


target_link_libraries(${PROJECT_NAME}
        PRIVATE glfw
        PRIVATE glm
        PRIVATE stb_image
        PRIVATE tinyobjloader
        PRIVATE Vulkan::Vulkan
        PRIVATE imgui
)


set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/Build/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/Build/Release
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    add_custom_target(copy_assets
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/Build/Debug/models
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/renderer/models
            ${CMAKE_SOURCE_DIR}/Build/Debug/models

            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/Build/Debug/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/renderer/shaders
            ${CMAKE_SOURCE_DIR}/Build/Debug/shaders

            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/Build/Debug/textures
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/renderer/textures
            ${CMAKE_SOURCE_DIR}/Build/Debug/textures
    )
elseif(${CMAKE_BUILD_TYPE} MATCHES "Release")
    add_custom_target(copy_assets
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/Build/Release/models
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/renderer/models
            ${CMAKE_SOURCE_DIR}/Build/Release/models

            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/Build/Release/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/renderer/shaders
            ${CMAKE_SOURCE_DIR}/Build/Release/shaders

            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/Build/Release/textures
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/renderer/textures
            ${CMAKE_SOURCE_DIR}/Build/Release/textures
    )
endif()

add_dependencies(${PROJECT_NAME} copy_assets)
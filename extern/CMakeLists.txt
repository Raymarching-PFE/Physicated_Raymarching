########## GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw)


########## GLM
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)

add_subdirectory(glm)


########## STB
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)


########## TINYOBJ
add_subdirectory(tinyobjloader)


########## IMGUI
add_library(imgui INTERFACE)
target_sources(imgui INTERFACE
        "imgui/imgui.cpp"
        "imgui/imgui_draw.cpp"
        "imgui/imgui_widgets.cpp"
        "imgui/imgui_tables.cpp"
        "imgui/backends/imgui_impl_glfw.cpp"
        "imgui/backends/imgui_impl_vulkan.cpp"
)
target_include_directories(imgui INTERFACE imgui)


########## TRACY
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/extern/tracy_executable)
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_SOURCE_DIR}/extern/tracy_executable.7z
            DESTINATION ${CMAKE_SOURCE_DIR}/extern
    )
endif()

include(FetchContent)
FetchContent_Declare(
        Tracy
        GIT_REPOSITORY https://github.com/wolfpld/tracy.git
        GIT_TAG v0.11.0
)
FetchContent_MakeAvailable(Tracy)

set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
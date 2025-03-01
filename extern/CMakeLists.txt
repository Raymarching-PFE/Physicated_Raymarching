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
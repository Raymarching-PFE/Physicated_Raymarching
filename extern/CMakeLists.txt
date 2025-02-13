########## GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INCLUDE_VULKAN ON CACHE BOOL "" FORCE)

add_subdirectory(glfw)


########## GLM
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)

add_subdirectory(glm)


########## STB
add_library(stb_image INTERFACE)
target_sources(stb_image INTERFACE "stb_image/stb_image.h")
target_include_directories(stb_image INTERFACE stb_image)


########## TINYOBJ
add_subdirectory(tinyobjloader)
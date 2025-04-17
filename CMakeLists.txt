cmake_minimum_required(VERSION 3.20)


if(NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()


project(Physicated_Raymarching)

add_subdirectory(extern) # EXTERNALS LIBRARIES
add_subdirectory(renderer)
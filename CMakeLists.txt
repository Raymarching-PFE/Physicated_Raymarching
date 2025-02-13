cmake_minimum_required(VERSION 3.20...3.31)

if(${CMAKE_VERSION} VERSION_LESS 3.25)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

set(CMAKE_CXX_STANDARD 20 CACHE STRING "Language standard" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Use the C++ standard" FORCE)
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "" FORCE)


find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                RESULT_VARIABLE GIT_SUBMOD_RESULT
        )
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/extern/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()


project(Physicated_Raymarching
        LANGUAGES CXX C
        VERSION 1.0
        DESCRIPTION "Physicated_Raymarching"
)


add_subdirectory(extern) # EXTERNALS LIBRARIES
add_subdirectory(renderer)
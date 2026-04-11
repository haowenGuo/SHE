cmake_minimum_required(VERSION 3.20)

project(
    SHE
    VERSION 0.1.0
    DESCRIPTION "Small 2D game engine bootstrap project"
    LANGUAGES CXX
)

include(cmake/SHEProjectOptions.cmake)

option(SHE_BUILD_SANDBOX "Build the engine sandbox executable" ON)
option(SHE_ENABLE_TESTS "Build smoke tests for the engine bootstrap" ON)

add_subdirectory(Engine)
add_subdirectory(Game)

if(SHE_BUILD_SANDBOX)
    add_subdirectory(Tools/Sandbox)
endif()

if(SHE_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

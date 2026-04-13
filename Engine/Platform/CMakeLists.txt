include(FetchContent)

set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Skip building SDL test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Skip building SDL test executables" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3)

add_library(
    she_engine_platform
    STATIC
        Source/SdlWindowService.cpp
        Source/NullWindowService.cpp
)

target_include_directories(she_engine_platform PUBLIC Include)
target_link_libraries(she_engine_platform PUBLIC she_engine_core SDL3::SDL3)
she_configure_target(she_engine_platform)


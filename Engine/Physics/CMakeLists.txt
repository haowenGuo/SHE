include(FetchContent)

set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "Skip Box2D unit tests" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "Skip Box2D testbed" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "Skip Box2D docs" FORCE)

FetchContent_Declare(
    box2d_external
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v2.4.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(box2d_external)

add_library(
    she_engine_physics
    STATIC
        Source/Box2DPhysicsService.cpp
        Source/NullPhysicsService.cpp
)

target_include_directories(she_engine_physics PUBLIC Include)
target_link_libraries(she_engine_physics PUBLIC she_engine_core box2d)
she_configure_target(she_engine_physics)


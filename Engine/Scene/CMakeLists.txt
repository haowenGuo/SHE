add_library(
    she_engine_scene
    STATIC
        Source/SceneWorld.cpp
)

target_include_directories(she_engine_scene PUBLIC Include)
target_link_libraries(she_engine_scene PUBLIC she_engine_core)
she_configure_target(she_engine_scene)


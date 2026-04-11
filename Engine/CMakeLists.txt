add_subdirectory(Core)
add_subdirectory(Platform)
add_subdirectory(Assets)
add_subdirectory(Scene)
add_subdirectory(Renderer)
add_subdirectory(Physics)
add_subdirectory(Audio)
add_subdirectory(UI)

add_library(she_engine INTERFACE)
target_link_libraries(
    she_engine
    INTERFACE
        she_engine_core
        she_engine_platform
        she_engine_assets
        she_engine_scene
        she_engine_renderer
        she_engine_physics
        she_engine_audio
        she_engine_ui
)


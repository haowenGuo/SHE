add_library(
    she_engine_renderer
    STATIC
        Source/Renderer2DService.cpp
)

target_include_directories(she_engine_renderer PUBLIC Include)
target_link_libraries(she_engine_renderer PUBLIC she_engine_core she_engine_platform)
she_configure_target(she_engine_renderer)


add_library(
    she_engine_renderer
    STATIC
        Source/NullRendererService.cpp
)

target_include_directories(she_engine_renderer PUBLIC Include)
target_link_libraries(she_engine_renderer PUBLIC she_engine_core)
she_configure_target(she_engine_renderer)


add_library(
    she_engine_ui
    STATIC
        Source/NullUiService.cpp
)

target_include_directories(she_engine_ui PUBLIC Include)
target_link_libraries(she_engine_ui PUBLIC she_engine_core)
she_configure_target(she_engine_ui)


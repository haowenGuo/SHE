add_library(
    she_engine_diagnostics
    STATIC
        Source/DiagnosticsService.cpp
)

target_include_directories(she_engine_diagnostics PUBLIC Include)
target_link_libraries(she_engine_diagnostics PUBLIC she_engine_core)
she_configure_target(she_engine_diagnostics)


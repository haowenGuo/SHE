add_library(
    she_engine_scripting
    STATIC
        Source/ScriptingService.cpp
)

target_include_directories(she_engine_scripting PUBLIC Include)
target_link_libraries(she_engine_scripting PUBLIC she_engine_core)
she_configure_target(she_engine_scripting)


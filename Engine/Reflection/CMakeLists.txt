add_library(
    she_engine_reflection
    STATIC
        Source/ReflectionService.cpp
)

target_include_directories(she_engine_reflection PUBLIC Include)
target_link_libraries(she_engine_reflection PUBLIC she_engine_core)
she_configure_target(she_engine_reflection)


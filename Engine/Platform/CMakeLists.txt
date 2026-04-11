add_library(
    she_engine_platform
    STATIC
        Source/NullWindowService.cpp
)

target_include_directories(she_engine_platform PUBLIC Include)
target_link_libraries(she_engine_platform PUBLIC she_engine_core)
she_configure_target(she_engine_platform)


add_library(
    she_engine_data
    STATIC
        Source/DataService.cpp
)

target_include_directories(she_engine_data PUBLIC Include)
target_link_libraries(she_engine_data PUBLIC she_engine_core)
she_configure_target(she_engine_data)


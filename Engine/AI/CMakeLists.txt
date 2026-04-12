add_library(
    she_engine_ai
    STATIC
        Source/AuthoringAiService.cpp
)

target_include_directories(she_engine_ai PUBLIC Include)
target_link_libraries(
    she_engine_ai
    PUBLIC
        she_engine_core
)
she_configure_target(she_engine_ai)


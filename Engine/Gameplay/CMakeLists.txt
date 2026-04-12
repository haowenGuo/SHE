add_library(
    she_engine_gameplay
    STATIC
        Source/CommandBuffer.cpp
        Source/GameplayService.cpp
        Source/TimerService.cpp
)

target_include_directories(she_engine_gameplay PUBLIC Include)
target_link_libraries(she_engine_gameplay PUBLIC she_engine_core)
she_configure_target(she_engine_gameplay)


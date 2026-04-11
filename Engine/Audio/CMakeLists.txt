add_library(
    she_engine_audio
    STATIC
        Source/NullAudioService.cpp
)

target_include_directories(she_engine_audio PUBLIC Include)
target_link_libraries(she_engine_audio PUBLIC she_engine_core)
she_configure_target(she_engine_audio)


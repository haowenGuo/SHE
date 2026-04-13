include(FetchContent)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.25
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(miniaudio)

add_library(
    she_engine_audio
    STATIC
        Source/AudioPlaybackContract.cpp
        Source/MiniaudioAudioService.cpp
        Source/NullAudioService.cpp
)

target_include_directories(she_engine_audio PUBLIC Include)
target_include_directories(she_engine_audio PRIVATE ${miniaudio_SOURCE_DIR})
target_compile_definitions(she_engine_audio PRIVATE SHE_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
target_link_libraries(
    she_engine_audio
    PUBLIC
        she_engine_core
    PRIVATE
        she_engine_assets
        she_engine_gameplay
)

if(WIN32)
    target_link_libraries(
        she_engine_audio
        PRIVATE
            avrt
            ole32
            uuid
            winmm
    )
endif()

she_configure_target(she_engine_audio)


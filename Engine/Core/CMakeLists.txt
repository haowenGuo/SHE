add_library(
    she_engine_core
    STATIC
        Source/AppClock.cpp
        Source/Application.cpp
        Source/LayerStack.cpp
        Source/Logger.cpp
)

target_include_directories(she_engine_core PUBLIC Include)
she_configure_target(she_engine_core)


add_library(
    she_engine_assets
    STATIC
        Source/AssetManager.cpp
)

target_include_directories(she_engine_assets PUBLIC Include)
target_link_libraries(she_engine_assets PUBLIC she_engine_core)
she_configure_target(she_engine_assets)


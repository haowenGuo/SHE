add_library(
    she_gameplay
    STATIC
        Features/Bootstrap/BootstrapFeatureLayer.cpp
        Features/FallingBlocks/FallingBlocksFeatureLayer.cpp
        Features/FallingBlocks/FallingBlocksGame.cpp
        Features/RogueLite/RogueLiteFeatureLayer.cpp
        Features/RogueLite/RogueLiteGame.cpp
        Features/VerticalSlice/VerticalSliceFeatureLayer.cpp
)

target_include_directories(she_gameplay PUBLIC Source Features)
target_link_libraries(she_gameplay PUBLIC she_engine)
she_configure_target(she_gameplay)

add_executable(
    she_game
    Source/Main.cpp
)

target_link_libraries(she_game PRIVATE she_gameplay she_engine)
she_configure_target(she_game)

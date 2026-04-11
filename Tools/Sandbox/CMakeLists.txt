add_executable(
    she_sandbox
    Source/Main.cpp
    Source/SandboxLayer.cpp
)

target_include_directories(she_sandbox PRIVATE Source)
target_link_libraries(she_sandbox PRIVATE she_engine)
she_configure_target(she_sandbox)


add_executable(
    she_tests
    Source/SmokeTests.cpp
)

target_link_libraries(she_tests PRIVATE she_engine)
she_configure_target(she_tests)

add_test(NAME she_smoke_tests COMMAND she_tests)

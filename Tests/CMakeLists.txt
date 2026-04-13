add_executable(
    she_tests
    Source/SmokeTests.cpp
)

target_link_libraries(she_tests PRIVATE she_engine)
she_configure_target(she_tests)

add_test(NAME she_smoke_tests COMMAND she_tests)

add_executable(
    she_gameplay_contract_tests
    Source/GameplayContractTests.cpp
)

target_link_libraries(she_gameplay_contract_tests PRIVATE she_engine)
she_configure_target(she_gameplay_contract_tests)

add_test(NAME she_gameplay_contract_tests COMMAND she_gameplay_contract_tests)

add_executable(
    she_data_contract_tests
    Source/DataContractTests.cpp
)

target_link_libraries(she_data_contract_tests PRIVATE she_engine_data she_engine_core)
she_configure_target(she_data_contract_tests)

add_test(NAME she_data_contract_tests COMMAND she_data_contract_tests)

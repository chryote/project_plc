add_executable(plc_tests
    main_test.cpp
    test_modbus_pdu.cpp
    test_haiwell_mapping.cpp
    test_domain_logic.cpp
    test_mock_client.cpp
)

target_link_libraries(plc_tests PRIVATE plc_core)
target_include_directories(plc_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME plc_unit_tests COMMAND plc_tests)

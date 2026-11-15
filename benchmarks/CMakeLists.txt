add_executable(plc_benchmarks
    main_benchmark.cpp
    benchmark_modbus_pdu.cpp
    benchmark_domain_math.cpp
)

target_link_libraries(plc_benchmarks PRIVATE plc_core)
target_include_directories(plc_benchmarks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.20)
project(plc_modbus_tool VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization and Warning flags
if(MSVC)
    add_compile_options(/W4 /WX- /permissive- /utf-8 /EHsc)
    # Enable Release optimizations
    add_compile_options("$<$<CONFIG:Release>:/O2;/Oi;/GL>")
    add_link_options("$<$<CONFIG:Release>:/LTCG>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    add_compile_options("$<$<CONFIG:Release>:-O3;-flto>")
    add_link_options("$<$<CONFIG:Release>:-flto>")
endif()

option(BUILD_TESTING "Build the test suite" ON)
option(BUILD_BENCHMARKS "Build the benchmark suite" ON)

# Core Library Sources
set(PLC_CORE_SOURCES
    src/network/socket.cpp
    src/modbus/modbus_pdu.cpp
    src/modbus/modbus_client.cpp
    src/domain/haiwell_mapping.cpp
    src/domain/side_slip.cpp
    src/domain/axle_load.cpp
    src/domain/speedometer.cpp
    src/domain/inspection_lane.cpp
    src/app/config.cpp
    src/app/formatter.cpp
    src/app/runner.cpp
)

add_library(plc_core STATIC ${PLC_CORE_SOURCES})

target_include_directories(plc_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(WIN32)
    target_link_libraries(plc_core PUBLIC ws2_32)
endif()

# Main CLI Executable
add_executable(plc_tool src/main.cpp)
target_link_libraries(plc_tool PRIVATE plc_core)

# Tests
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Benchmarks
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

# Installation rules
install(TARGETS plc_tool plc_core
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

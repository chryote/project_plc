#include "benchmark_framework.hpp"
#include "plc/modbus/modbus_pdu.hpp"
#include "plc/modbus/modbus_mock.hpp"

namespace plc::bench {

void run_modbus_benchmarks() {
    std::cout << "\n--- Modbus Protocol & Serialization Benchmarks ---\n";
    std::cout << std::left << std::setw(38) << "Benchmark Name"
              << std::right << std::setw(12) << "Iterations"
              << std::setw(15) << "Avg Latency"
              << std::setw(21) << "Throughput\n";
    std::cout << std::string(86, '-') << "\n";

    // 1. Build Read Holding Registers Request
    BenchmarkHarness::run("ModbusPdu::build_request", 2'000'000, []() {
        auto req = modbus::ModbusPdu::build_read_holding_registers_request(100, 1, 1110, 4);
        (void)req;
    });

    // 2. Parse MBAP Header
    std::vector<core::Byte> mbap_raw = {0x00, 0x64, 0x00, 0x00, 0x00, 0x0B, 0x01};
    BenchmarkHarness::run("ModbusPdu::parse_mbap_header", 5'000'000, [&]() {
        auto res = modbus::ModbusPdu::parse_mbap_header(mbap_raw.data(), mbap_raw.size());
        (void)res;
    });

    // 3. Parse Holding Registers Payload (10 registers = 20 bytes)
    std::vector<core::Byte> payload_raw(20, 0x01);
    BenchmarkHarness::run("ModbusPdu::parse_payload (10 regs)", 2'000'000, [&]() {
        auto res = modbus::ModbusPdu::parse_read_holding_registers_payload(payload_raw.data(), payload_raw.size(), 10);
        (void)res;
    });

    // 4. Mock Modbus Client Round-Trip
    modbus::MockModbusClient mock;
    mock.connect();
    BenchmarkHarness::run("MockModbusClient::read (in-memory)", 500'000, [&]() {
        auto res = mock.read_holding_registers(1110, 4);
        (void)res;
    });
}

} // namespace plc::bench

#include "plc/app/runner.hpp"
#include "plc/app/formatter.hpp"
#include "plc/core/logger.hpp"
#include "plc/modbus/modbus_defs.hpp"
#include "plc/modbus/modbus_pdu.hpp"
#include "plc/modbus/modbus_client.hpp"
#include "plc/modbus/modbus_mock.hpp"
#include "plc/domain/inspection_lane.hpp"
#include "plc/domain/haiwell_mapping.hpp"

#include <iostream>
#include <thread>
#include <csignal>
#include <atomic>
#include <chrono>

namespace plc::app {

namespace {
std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}
} // namespace

Runner::Runner(AppConfig config) : config_(std::move(config)) {}

int Runner::run() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    switch (config_.command) {
        case Command::Lane:
            return execute_lane();
        case Command::Read:
            return execute_read();
        case Command::Probe:
            return execute_probe();
        case Command::Benchmark:
            return execute_benchmark();
        case Command::Help:
            AppConfig::print_usage("plc_tool");
            return 0;
        case Command::Version:
            std::cout << "plc_tool version 2.0.0 (Modern C++20 Clean Architecture)\n";
            return 0;
    }
    return 0;
}

int Runner::execute_lane() {
    std::unique_ptr<modbus::IModbusClient> client;
    if (config_.use_mock) {
        LOG_INFO("Running in MOCK mode (simulated PLC registers)");
        client = std::make_unique<modbus::MockModbusClient>(config_.connection);
    } else {
        client = std::make_unique<modbus::ModbusTcpClient>(config_.connection);
    }

    auto conn_res = client->connect();
    if (!conn_res) {
        std::cerr << "[!] Error connecting to PLC: " << conn_res.error().to_string() << "\n";
        return 1;
    }

    domain::InspectionLaneService service(*client, config_.addressing_mode);

    do {
        auto snap_res = service.read_full_snapshot();
        if (!snap_res) {
            std::cerr << "[!] Failed to read inspection lane snapshot: "
                      << snap_res.error().to_string() << "\n";
            if (!config_.continuous) return 1;
        } else {
            switch (config_.output_format) {
                case OutputFormat::Text:
                    std::cout << Formatter::format_lane_text(snap_res.value());
                    break;
                case OutputFormat::Json:
                    std::cout << Formatter::format_lane_json(snap_res.value()) << "\n";
                    break;
                case OutputFormat::Csv:
                    std::cout << Formatter::format_lane_csv(snap_res.value());
                    break;
            }
        }

        if (config_.continuous && g_running) {
            std::this_thread::sleep_for(config_.interval);
        }
    } while (config_.continuous && g_running);

    return 0;
}

int Runner::execute_read() {
    std::unique_ptr<modbus::IModbusClient> client;
    if (config_.use_mock) {
        LOG_INFO("Running in MOCK mode (simulated PLC registers)");
        client = std::make_unique<modbus::MockModbusClient>(config_.connection);
    } else {
        client = std::make_unique<modbus::ModbusTcpClient>(config_.connection);
    }

    auto conn_res = client->connect();
    if (!conn_res) {
        std::cerr << "[!] Error connecting to PLC: " << conn_res.error().to_string() << "\n";
        return 1;
    }

    core::RegisterAddress addr = config_.read_start_addr;
    core::RegisterCount count = config_.read_count;

    if (!config_.read_symbol.empty()) {
        try {
            addr = domain::HaiwellMapping::to_modbus_address(config_.read_symbol, config_.addressing_mode);
        } catch (const std::exception& e) {
            std::cerr << "[!] Symbol resolution error: " << e.what() << "\n";
            return 1;
        }
    }

    domain::InspectionLaneService service(*client, config_.addressing_mode);

    if (!config_.station_name.empty()) {
        if (config_.station_name == "sideslip") {
            auto res = service.read_side_slip();
            if (!res) {
                std::cerr << "[!] Error: " << res.error().to_string() << "\n";
                return 1;
            }
            std::cout << "Side Slip Raw:    " << res.value().raw_adc << "\n"
                      << "Side Slip Scaled: " << res.value().plc_scaled << " mm/m\n"
                      << "Calculated:       " << res.value().calculated_scaled << " mm/m\n";
            return 0;
        } else if (config_.station_name == "axle") {
            auto res = service.read_axle_load();
            if (!res) {
                std::cerr << "[!] Error: " << res.error().to_string() << "\n";
                return 1;
            }
            std::cout << "CH1 Left Live:    " << res.value().ch1_left.live_weight << " kg (Avg: " << res.value().ch1_left.avg_weight << ")\n"
                      << "CH2 Right Live:   " << res.value().ch2_right.live_weight << " kg (Avg: " << res.value().ch2_right.avg_weight << ")\n"
                      << "Sumbu 1..4:       [" << res.value().sumbu_weights[0] << ", "
                      << res.value().sumbu_weights[1] << ", "
                      << res.value().sumbu_weights[2] << ", "
                      << res.value().sumbu_weights[3] << "] kg\n";
            return 0;
        } else if (config_.station_name == "speed") {
            auto res = service.read_speedometer();
            if (!res) {
                std::cerr << "[!] Error: " << res.error().to_string() << "\n";
                return 1;
            }
            std::cout << "Speed Display:    " << res.value().display_speed << "\n"
                      << "Encoder PPR:      " << res.value().encoder_ppr << "\n"
                      << "Raw Pulses:       " << res.value().raw_pulses << "\n"
                      << "Pulse Per Min:    " << res.value().pulse_per_minute << "\n"
                      << "Estimated km/h:   " << res.value().estimated_kmh() << " km/h\n";
            return 0;
        } else {
            std::cerr << "[!] Unknown station: " << config_.station_name << " (use sideslip, axle, or speed)\n";
            return 1;
        }
    }

    auto regs_res = client->read_holding_registers(addr, count);
    if (!regs_res) {
        std::cerr << "[!] Read error: " << regs_res.error().to_string() << "\n";
        return 1;
    }

    switch (config_.output_format) {
        case OutputFormat::Text:
            std::cout << Formatter::format_registers_text(addr, regs_res.value(), config_.addressing_mode);
            break;
        case OutputFormat::Json:
            std::cout << Formatter::format_registers_json(addr, regs_res.value(), config_.addressing_mode);
            break;
        case OutputFormat::Csv:
            for (std::size_t i = 0; i < regs_res.value().size(); ++i) {
                std::cout << (addr + i) << "," << regs_res.value()[i] << "\n";
            }
            break;
    }

    return 0;
}

int Runner::execute_probe() {
    std::unique_ptr<modbus::IModbusClient> client;
    if (config_.use_mock) {
        LOG_INFO("Running probe in MOCK mode");
        client = std::make_unique<modbus::MockModbusClient>(config_.connection);
    } else {
        client = std::make_unique<modbus::ModbusTcpClient>(config_.connection);
    }

    auto conn_res = client->connect();
    if (!conn_res) {
        std::cerr << "[!] Error connecting to PLC: " << conn_res.error().to_string() << "\n";
        return 1;
    }

    domain::InspectionLaneService service(*client, config_.addressing_mode);

    std::cout << "Scanning registers from " << config_.probe_start
              << " to " << (config_.probe_start + config_.probe_count - 1) << "...\n";

    auto probe_res = service.probe_range(config_.probe_start, config_.probe_count, config_.probe_delay);
    if (!probe_res) {
        std::cerr << "[!] Probe error: " << probe_res.error().to_string() << "\n";
        return 1;
    }

    switch (config_.output_format) {
        case OutputFormat::Text:
            std::cout << Formatter::format_probe_text(probe_res.value());
            break;
        case OutputFormat::Json:
            std::cout << Formatter::format_probe_json(probe_res.value()) << "\n";
            break;
        case OutputFormat::Csv:
            std::cout << "symbol,address,snap1,snap2,fluctuating\n";
            for (const auto& e : probe_res.value()) {
                std::cout << e.symbol << "," << e.address << "," << e.snap1 << "," << e.snap2 << ","
                          << (e.is_fluctuating ? 1 : 0) << "\n";
            }
            break;
    }

    return 0;
}

int Runner::execute_benchmark() {
    std::cout << "============================================================\n"
              << "          PLC MODBUS TOOLING MICROBENCHMARK                 \n"
              << "============================================================\n";

    // 1. Benchmark Modbus PDU Framing
    constexpr std::size_t ITERATIONS = 1'000'000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < ITERATIONS; ++i) {
        auto req = modbus::ModbusPdu::build_read_holding_registers_request(
            static_cast<core::TransactionId>(i & 0xFFFF),
            1,
            static_cast<core::RegisterAddress>(i % 1000),
            10);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double pdu_ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / ITERATIONS;
    double pdu_ops = (ITERATIONS / std::chrono::duration<double>(t1 - t0).count());

    std::cout << "[1] Modbus PDU Request Framing:\n"
              << "    - Operations: " << ITERATIONS << "\n"
              << "    - Latency:    " << pdu_ns << " ns/op\n"
              << "    - Throughput: " << std::fixed << std::setprecision(0) << pdu_ops << " ops/sec\n\n";

    // 2. Benchmark Linear Scaling Math
    domain::SideSlipCalibration calib{130, 30, -1, 30};
    auto t2 = std::chrono::high_resolution_clock::now();
    double dummy_sum = 0.0;
    for (std::size_t i = 0; i < ITERATIONS; ++i) {
        dummy_sum += domain::SideSlipCalculator::calculate_scaled(static_cast<std::int16_t>(i % 300), calib);
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    double math_ns = std::chrono::duration<double, std::nano>(t3 - t2).count() / ITERATIONS;
    double math_ops = (ITERATIONS / std::chrono::duration<double>(t3 - t2).count());

    std::cout << "[2] Domain Linear Scaling Math:\n"
              << "    - Operations: " << ITERATIONS << " (result check: " << dummy_sum << ")\n"
              << "    - Latency:    " << math_ns << " ns/op\n"
              << "    - Throughput: " << math_ops << " ops/sec\n\n";

    // 3. Mock Client In-Memory Round-Trip
    constexpr std::size_t MOCK_ITERS = 100'000;
    modbus::MockModbusClient mock_client;
    mock_client.connect();
    auto t4 = std::chrono::high_resolution_clock::now();
    for (std::size_t i = 0; i < MOCK_ITERS; ++i) {
        auto res = mock_client.read_holding_registers(1110, 4);
    }
    auto t5 = std::chrono::high_resolution_clock::now();
    double mock_ns = std::chrono::duration<double, std::nano>(t5 - t4).count() / MOCK_ITERS;
    double mock_ops = (MOCK_ITERS / std::chrono::duration<double>(t5 - t4).count());

    std::cout << "[3] Mock Modbus In-Memory Round-Trip:\n"
              << "    - Operations: " << MOCK_ITERS << "\n"
              << "    - Latency:    " << mock_ns << " ns/op\n"
              << "    - Throughput: " << mock_ops << " reads/sec\n";

    std::cout << "============================================================\n";
    return 0;
}

} // namespace plc::app

#pragma once

#include "plc/core/types.hpp"
#include "plc/domain/haiwell_mapping.hpp"

#include <string>
#include <vector>

namespace plc::app {

enum class Command {
    Read,
    Lane,
    Probe,
    Benchmark,
    Help,
    Version
};

enum class OutputFormat {
    Text,
    Json,
    Csv
};

struct AppConfig {
    Command command = Command::Lane;
    core::ConnectionConfig connection{};
    domain::AddressingMode addressing_mode = domain::AddressingMode::DirectZeroBased;
    OutputFormat output_format = OutputFormat::Text;

    // Command-specific options
    core::RegisterAddress read_start_addr = 0;
    core::RegisterCount read_count = 1;
    std::string read_symbol;
    std::string station_name; // "sideslip", "axle", "speed"

    core::RegisterAddress probe_start = 0;
    core::RegisterCount probe_count = 2500;
    core::Milliseconds probe_delay = core::Milliseconds(500);

    bool continuous = false;
    core::Milliseconds interval = core::Milliseconds(1000);
    bool use_mock = false;
    bool verbose = false;

    static AppConfig parse(int argc, char* argv[]);
    static void print_usage(const char* prog_name);
};

} // namespace plc::app

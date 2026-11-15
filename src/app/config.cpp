#include "plc/app/config.hpp"
#include "plc/core/logger.hpp"

#include <iostream>
#include <cstdlib>
#include <string_view>
#include <algorithm>

namespace plc::app {

namespace {

std::string get_env_or_default(const char* name, const std::string& default_val) {
#if defined(_WIN32)
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, name) == 0 && buf != nullptr) {
        std::string val(buf);
        free(buf);
        return val;
    }
#else
    const char* val = std::getenv(name);
    if (val != nullptr) {
        return std::string(val);
    }
#endif
    return default_val;
}

std::string to_lower(std::string_view s) {
    std::string res;
    res.reserve(s.size());
    for (char c : s) {
        res.push_back(static_cast<char>(std::tolower(c)));
    }
    return res;
}

} // namespace

AppConfig AppConfig::parse(int argc, char* argv[]) {
    AppConfig config;

    // Load environment variables if available
    config.connection.host = get_env_or_default("PLC_HOST", "192.168.70.111");
    config.connection.port = static_cast<std::uint16_t>(std::atoi(get_env_or_default("PLC_PORT", "502").c_str()));
    config.connection.unit_id = static_cast<core::UnitId>(std::atoi(get_env_or_default("PLC_UNIT_ID", "1").c_str()));

    if (argc < 2) {
        config.command = Command::Lane;
        return config;
    }

    int i = 1;
    std::string first_arg = argv[i];

    if (first_arg == "read") {
        config.command = Command::Read;
        ++i;
    } else if (first_arg == "lane") {
        config.command = Command::Lane;
        ++i;
    } else if (first_arg == "probe") {
        config.command = Command::Probe;
        ++i;
    } else if (first_arg == "benchmark") {
        config.command = Command::Benchmark;
        ++i;
    } else if (first_arg == "help" || first_arg == "--help" || first_arg == "-h") {
        config.command = Command::Help;
        return config;
    } else if (first_arg == "version" || first_arg == "--version" || first_arg == "-v") {
        config.command = Command::Version;
        return config;
    }

    for (; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "--host" && i + 1 < argc) {
            config.connection.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            config.connection.port = static_cast<std::uint16_t>(std::atoi(argv[++i]));
        } else if (arg == "--unit" && i + 1 < argc) {
            config.connection.unit_id = static_cast<core::UnitId>(std::atoi(argv[++i]));
        } else if (arg == "--timeout" && i + 1 < argc) {
            config.connection.timeout = core::Milliseconds(std::atoi(argv[++i]));
        } else if (arg == "--addr" && i + 1 < argc) {
            config.read_start_addr = static_cast<core::RegisterAddress>(std::atoi(argv[++i]));
        } else if (arg == "--count" && i + 1 < argc) {
            config.read_count = static_cast<core::RegisterCount>(std::atoi(argv[++i]));
        } else if (arg == "--symbol" && i + 1 < argc) {
            config.read_symbol = argv[++i];
        } else if (arg == "--station" && i + 1 < argc) {
            config.station_name = to_lower(argv[++i]);
        } else if (arg == "--start" && i + 1 < argc) {
            config.probe_start = static_cast<core::RegisterAddress>(std::atoi(argv[++i]));
        } else if (arg == "--probe-count" && i + 1 < argc) {
            config.probe_count = static_cast<core::RegisterCount>(std::atoi(argv[++i]));
        } else if (arg == "--addressing" && i + 1 < argc) {
            std::string mode_str = to_lower(argv[++i]);
            if (mode_str == "haiwell-offset" || mode_str == "offset") {
                config.addressing_mode = domain::AddressingMode::HaiwellOffset;
            } else {
                config.addressing_mode = domain::AddressingMode::DirectZeroBased;
            }
        } else if (arg == "--format" && i + 1 < argc) {
            std::string fmt = to_lower(argv[++i]);
            if (fmt == "json") {
                config.output_format = OutputFormat::Json;
            } else if (fmt == "csv") {
                config.output_format = OutputFormat::Csv;
            } else {
                config.output_format = OutputFormat::Text;
            }
        } else if (arg == "--continuous" || arg == "-c") {
            config.continuous = true;
        } else if (arg == "--interval" && i + 1 < argc) {
            config.interval = core::Milliseconds(std::atoi(argv[++i]));
        } else if (arg == "--mock") {
            config.use_mock = true;
        } else if (arg == "--verbose") {
            config.verbose = true;
            core::Logger::instance().set_level(core::LogLevel::Debug);
        } else if (arg == "--help" || arg == "-h") {
            config.command = Command::Help;
            return config;
        } else if (arg == "--version" || arg == "-v") {
            config.command = Command::Version;
            return config;
        }
    }

    return config;
}

void AppConfig::print_usage(const char* prog_name) {
    std::cout << "PLC Modbus Tooling - High Performance C++ PLC Communication Tool\n\n"
              << "Usage: " << prog_name << " [command] [options]\n\n"
              << "Commands:\n"
              << "  lane           Read snapshot or stream live readings for vehicle inspection lane (default)\n"
              << "  read           Read holding registers by address, symbol (e.g. V1110), or station\n"
              << "  probe          Probe register range to detect active, non-zero, or fluctuating registers\n"
              << "  benchmark      Run built-in communication / parsing throughput benchmarks\n"
              << "  help           Display this help screen\n"
              << "  version        Display version information\n\n"
              << "Options:\n"
              << "  --host <ip>             PLC IP address (default: 192.168.70.111 or $PLC_HOST)\n"
              << "  --port <port>           PLC Modbus TCP port (default: 502 or $PLC_PORT)\n"
              << "  --unit <id>             Modbus Slave / Unit ID (default: 1 or $PLC_UNIT_ID)\n"
              << "  --timeout <ms>          Socket connection/read timeout in milliseconds (default: 3000)\n"
              << "  --addr <n>              Starting register address for 'read' command\n"
              << "  --count <n>             Number of registers to read (default: 1)\n"
              << "  --symbol <sym>          Read by Haiwell symbol (e.g. V1110, V1232, AI0)\n"
              << "  --station <name>        Read specific station: sideslip | axle | speed\n"
              << "  --addressing <mode>     Addressing mode: 'direct' (V<n>=n) or 'haiwell-offset' (V<n>=512+n)\n"
              << "  --start <n>             Starting address for 'probe' command (default: 0)\n"
              << "  --probe-count <n>       Register count for 'probe' command (default: 2500)\n"
              << "  --format <fmt>          Output format: text | json | csv (default: text)\n"
              << "  -c, --continuous        Stream readings continuously in loop\n"
              << "  --interval <ms>         Streaming refresh interval in milliseconds (default: 1000)\n"
              << "  --mock                  Run against built-in simulated PLC mock\n"
              << "  --verbose               Enable verbose debug logging\n"
              << "  -h, --help              Show help information\n"
              << "  -v, --version           Show version information\n";
}

} // namespace plc::app

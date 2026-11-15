#include "benchmark_framework.hpp"

namespace plc::bench {
    void run_modbus_benchmarks();
    void run_domain_benchmarks();
}

int main() {
    std::cout << "======================================================================================\n"
              << "                         PLC MODBUS TOOLING BENCHMARK SUITE                           \n"
              << "======================================================================================\n";

    plc::bench::run_modbus_benchmarks();
    plc::bench::run_domain_benchmarks();

    std::cout << "\n======================================================================================\n"
              << "Benchmark completed successfully.\n";
    return 0;
}

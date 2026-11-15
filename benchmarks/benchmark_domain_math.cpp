#include "benchmark_framework.hpp"
#include "plc/domain/side_slip.hpp"
#include "plc/domain/axle_load.hpp"
#include "plc/domain/speedometer.hpp"

namespace plc::bench {

void run_domain_benchmarks() {
    std::cout << "\n--- Domain Logic & Calculation Benchmarks ---\n";
    std::cout << std::left << std::setw(38) << "Benchmark Name"
              << std::right << std::setw(12) << "Iterations"
              << std::setw(15) << "Avg Latency"
              << std::setw(21) << "Throughput\n";
    std::cout << std::string(86, '-') << "\n";

    // 1. Side Slip Linear Scaling Math
    domain::SideSlipCalibration calib{130, 30, -1, 30};
    volatile double sink_f64 = 0.0;
    BenchmarkHarness::run("SideSlip::calculate_scaled", 10'000'000, [&]() {
        sink_f64 = domain::SideSlipCalculator::calculate_scaled(131, calib);
    });

    // 2. Axle Load Side Difference Calculation
    domain::AxleLoadReading axle;
    axle.ch1_left.live_weight = 512;
    axle.ch2_right.live_weight = 548;
    BenchmarkHarness::run("AxleLoad::side_difference_percent", 10'000'000, [&]() {
        sink_f64 = axle.side_difference_percent();
    });

    // 3. Speedometer RPM & km/h Calculation
    domain::SpeedometerReading speed;
    speed.encoder_ppr = 360;
    speed.pulse_per_minute = 18000;
    BenchmarkHarness::run("Speedometer::estimated_kmh", 10'000'000, [&]() {
        sink_f64 = speed.estimated_kmh(0.35);
    });
}

} // namespace plc::bench

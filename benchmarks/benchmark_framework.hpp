#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <iomanip>
#include <numeric>
#include <algorithm>

namespace plc::bench {

struct BenchmarkResult {
    std::string name;
    std::size_t iterations = 0;
    double duration_seconds = 0.0;
    double avg_ns = 0.0;
    double ops_per_sec = 0.0;
};

class BenchmarkHarness {
public:
    static BenchmarkResult run(const std::string& name, std::size_t iterations, std::function<void()> func) {
        // Warmup
        for (std::size_t i = 0; i < std::min<std::size_t>(iterations / 10, 1000); ++i) {
            func();
        }

        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();

        double dur = std::chrono::duration<double>(end - start).count();
        double avg_ns = std::chrono::duration<double, std::nano>(end - start).count() / static_cast<double>(iterations);
        double ops = (dur > 0.0) ? (static_cast<double>(iterations) / dur) : 0.0;

        BenchmarkResult res{name, iterations, dur, avg_ns, ops};
        print_result(res);
        return res;
    }

private:
    static void print_result(const BenchmarkResult& res) {
        std::cout << std::left << std::setw(38) << res.name
                  << std::right << std::setw(12) << res.iterations
                  << std::setw(12) << std::fixed << std::setprecision(1) << res.avg_ns << " ns"
                  << std::setw(15) << std::fixed << std::setprecision(0) << res.ops_per_sec << " ops/s"
                  << "\n";
    }
};

} // namespace plc::bench
